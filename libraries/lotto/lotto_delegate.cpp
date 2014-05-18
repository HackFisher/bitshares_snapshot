#include <bts/lotto/lotto_delegate.hpp>
#include <bts/lotto/lotto_wallet.hpp>
#include <bts/db/level_map.hpp>

#include <fc/thread/thread.hpp>
#include <fc/log/logger.hpp>

namespace bts { namespace lotto {

    namespace detail
    {
        class lotto_delegate_impl
        {
            public:
                lotto_delegate_impl(){}

                lotto_delegate*                                     _self;

                lotto_db_ptr                                        _lotto_db;
                lotto_wallet_ptr                                    _lotto_wallet;
                bts::client::client_ptr                             _client;

                std::pair<fc::sha256, fc::sha256> delegate_secret_last_revealed_secret_pair(uint16_t delegate_id);

                void secret_loop();

                void generate_broadcast_next_secret();

                fc::ecc::private_key                                        _trustee_key;
                std::string                                                 _wallet_pass;
                fc::time_point                                              _last_block_timestamp;

                fc::future<void>                                            _secret_loop_complete;

                fc::path                                                    _data_dir;
                bts::db::level_map<fc::sha256, fc::sha256>                  _secret2revealed_secret;
        };

        std::pair<fc::sha256, fc::sha256> lotto_delegate_impl::delegate_secret_last_revealed_secret_pair(uint16_t delegate_id)
        {
            if (delegate_id == 0)   // reset
            {
                //_delegate2last_secret.clear();
                return std::pair<fc::sha256, fc::sha256>(fc::sha256(), fc::sha256());
            }

            if (_lotto_db->is_new_delegate(delegate_id))
            {
                fc::sha256 new_reveal_secret = fc::sha256("FFFEEE" + fc::to_string((int64_t)std::rand()));
                fc::sha256 new_secret = fc::sha256::hash(new_reveal_secret);
                _secret2revealed_secret.store(new_secret, new_reveal_secret);

                return std::pair<fc::sha256, fc::sha256>(new_secret, fc::sha256());
            }
            else
            {
                fc::sha256 new_reveal_secret = fc::sha256("FFFEEE" + fc::to_string((int64_t)std::rand()));
                fc::sha256 new_secret = fc::sha256::hash(new_reveal_secret);
                _secret2revealed_secret.store(new_secret, new_reveal_secret);

                auto blk_ids = _lotto_db->fetch_blocks_idxs(delegate_id);
                FC_ASSERT(blk_ids.size() > 0);

                auto last_secret_by_delegate = _lotto_db->fetch_secret(blk_ids[blk_ids.size() - 1]);
                auto last_secret = last_secret_by_delegate.secret;
                auto last_revealed_secret = _secret2revealed_secret.fetch(last_secret);

                auto secret_pair = std::pair<fc::sha256, fc::sha256>(new_secret, last_revealed_secret);
                return secret_pair;
            }
        }

        void lotto_delegate_impl::generate_broadcast_next_secret()
        {
            FC_ASSERT(!_lotto_wallet->is_locked());

            uint32_t delegate_id = _lotto_db->get_head_block_num() % BTS_BLOCKCHAIN_NUM_DELEGATES + 1;
            auto secret_pair = delegate_secret_last_revealed_secret_pair(delegate_id);
            // TODO: maybe move from wallet to here if no fees
            auto secret_trx = _lotto_wallet->next_secret(secret_pair.first, secret_pair.second, delegate_id, address(_trustee_key.get_public_key()));
            _client->broadcast_transaction(secret_trx);
        }

        void lotto_delegate_impl::secret_loop()
        {
            try {
                _secret2revealed_secret.open(_data_dir / "delegate_data" / "secret", true);
            } FC_RETHROW_EXCEPTIONS(warn, "Error loading delegate secret database ${dir}", ("dir", "delegate_data")("create", true));

            fc::usleep(fc::seconds(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC/2));
            _last_block_timestamp = _lotto_db->get_head_block().timestamp;

            try {
                while (!_secret_loop_complete.canceled() && _lotto_wallet->is_locked())
                {
                    fc::usleep(fc::seconds(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC / 2));
                }
                // generate secret needs trustee key, and need to unlock before import key.
                /*
                if (_self->get_wallet()->is_locked())
                {
                    wlog("start unlock the wallet before secret loop..");
                    _self->get_wallet()->unlock_wallet(_wallet_pass);
                    wlog("unlock the wallet before secret loop..");
                }
                */

                _lotto_wallet->import_private_key(_trustee_key, "import trustee key", "trustee");
                _lotto_wallet->scan_chain(0);

                // broadcast a secret
                generate_broadcast_next_secret();
            }
            catch (const fc::exception& e)
            {
                elog("error broadcasting next secret?: ${e}", ("e", e.to_detail_string()));
            }

            while (!_secret_loop_complete.canceled())
            {
                auto new_block_time = _lotto_db->get_head_block().timestamp;

                // new block is out
                if (new_block_time > _last_block_timestamp)
                {
                    try {
                        // broadcast new secret
                        generate_broadcast_next_secret();

                        // generate secret could fail due to wallet lock, so move below to keep trying after unlock wallet
                        _last_block_timestamp = new_block_time;
                    }
                    catch (const fc::exception& e)
                    {
                        elog("error broadcasting next secret?: ${e}", ("e", e.to_detail_string()));
                    }
                }

                fc::usleep(fc::seconds(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC / 2));
            }

            _secret2revealed_secret.close();
        }
    }

    lotto_delegate::lotto_delegate()
        :my(new detail::lotto_delegate_impl())
    {
        my->_self = this;
    }

    lotto_delegate::~lotto_delegate()
    {
        try {
            if (my->_secret_loop_complete.valid())
            {
                my->_secret_loop_complete.cancel();
                ilog("waiting for secret loop to complete");
                my->_secret_loop_complete.wait();
            }
        }
        catch (const fc::canceled_exception&) {}
        catch (const fc::exception& e)
        {
            wlog("${e}", ("e", e.to_detail_string()));
        }
    }

    void lotto_delegate::set_lotto_db(const bts::lotto::lotto_db_ptr& ptr)
    {
        my->_lotto_db = ptr;
    }

    void lotto_delegate::set_lotto_wallet(const bts::lotto::lotto_wallet_ptr& wall)
    {
        FC_ASSERT(my->_lotto_db);
        my->_lotto_wallet = wall;
    }

    void lotto_delegate::set_client(const bts::client::client_ptr client)
    {
        my->_client = client;
    }

    void lotto_delegate::run_secret_broadcastor(const fc::ecc::private_key& k, const std::string& wallet_pass, const fc::path& datadir)
    {
        my->_trustee_key = k;
        my->_wallet_pass = wallet_pass;
        my->_secret_loop_complete = fc::async([=](){ my->secret_loop(); });

        my->_data_dir = datadir;
    }

} } // bts::lotto