#include <bts/lotto/lotto_client.hpp>
#include <bts/lotto/lotto_wallet.hpp>

#include <fc/thread/thread.hpp>
#include <fc/log/logger.hpp>

namespace bts { namespace lotto {

    std::pair<fc::sha256, fc::sha256> delegate_secret_last_revealed_secret_pair(uint16_t delegate_id)
    {
        static std::map<uint16_t, std::pair<fc::sha256, fc::sha256>> _delegate_secret_pair_map;

        if (delegate_id == 0)   // reset
        {
            _delegate_secret_pair_map.clear();
            return std::pair<fc::sha256, fc::sha256>(fc::sha256(), fc::sha256());
        }

        if (_delegate_secret_pair_map.find(delegate_id) == _delegate_secret_pair_map.end())
        {
            fc::sha256 new_reveal_secret = fc::sha256("FFFEEE" + fc::to_string((int64_t)std::rand()));
            fc::sha256 new_secret = fc::sha256::hash(new_reveal_secret);
            _delegate_secret_pair_map[delegate_id] = std::pair<fc::sha256, fc::sha256>(new_secret, new_reveal_secret);
            return std::pair<fc::sha256, fc::sha256>(new_secret, fc::sha256());
        }
        else
        {
            fc::sha256 new_reveal_secret = fc::sha256("FFFEEE" + fc::to_string((int64_t)std::rand()));
            fc::sha256 new_secret = fc::sha256::hash(new_reveal_secret);
            auto secret_pair = std::pair<fc::sha256, fc::sha256>(new_secret, _delegate_secret_pair_map[delegate_id].second);
            _delegate_secret_pair_map[delegate_id] = std::pair<fc::sha256, fc::sha256>(new_secret, new_reveal_secret);
            return secret_pair;
        }
    }

    namespace detail
    {
        class lotto_client_impl
        {
            public:
                lotto_client_impl(){}

                lotto_client*                                     _self;

                const lotto_wallet_ptr get_lotto_wallet()
                {
                    return std::dynamic_pointer_cast<lotto_wallet>(_self->get_wallet());
                }

                void secret_loop();

                void generate_broadcast_next_secret();

                fc::ecc::private_key                                        _trustee_key;
                std::string                                                 _wallet_pass;
                fc::time_point                                              _last_block_timestamp;

                fc::future<void>                                            _secret_loop_complete;
        };

        void lotto_client_impl::generate_broadcast_next_secret()
        {
            FC_ASSERT(!_self->get_wallet()->is_locked());

            uint32_t delegate_id = _self->get_chain()->head_block_num() % 100 + 1;
            auto secret_pair = delegate_secret_last_revealed_secret_pair(delegate_id);
            auto secret_trx = get_lotto_wallet()->next_secret(secret_pair.first, secret_pair.second, delegate_id, address(_trustee_key.get_public_key()));
            _self->broadcast_transaction(secret_trx);
        }

        void lotto_client_impl::secret_loop()
        {
            fc::usleep(fc::seconds(10));
            _last_block_timestamp = _self->get_chain()->get_head_block().timestamp;
            // generate secret needs trustee key, and need to unlock before import key.
            if (_self->get_wallet()->is_locked())
            {
                _self->get_wallet()->unlock_wallet(_wallet_pass);
            }
            _self->get_wallet()->import_key(_trustee_key, "import trustee key", "trustee");

            // broadcast a secret
            generate_broadcast_next_secret();

            while (!_secret_loop_complete.canceled())
            {
                auto new_block_time = _self->get_chain()->get_head_block().timestamp;

                // new block is out
                if (new_block_time > _last_block_timestamp)
                {
                    _last_block_timestamp = new_block_time;
                    
                    // broadcast new secret
                    generate_broadcast_next_secret();
                }

                fc::usleep(fc::seconds(BTS_BLOCKCHAIN_BLOCK_INTERVAL_SEC / 3));
            }
        }
    }

    void lotto_client::run_secret_broadcastor(const fc::ecc::private_key& k, const std::string& wallet_pass)
    {
        my->_trustee_key = k;
        my->_wallet_pass = wallet_pass;
        my->_secret_loop_complete = fc::async([=](){ my->secret_loop(); });
    }

    lotto_client::lotto_client(bool enable_p2p /* = false */)
        :my(new detail::lotto_client_impl())
    {
        my->_self = this;
    }

    lotto_client::~lotto_client()
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

} } // bts::lotto