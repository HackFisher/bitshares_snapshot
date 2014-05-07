#define BOOST_TEST_MODULE LottoTests
#include <boost/test/unit_test.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/blockchain/block_miner.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/lotto/lotto_wallet.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <iostream>
using namespace bts::wallet;
using namespace bts::blockchain;
using namespace bts::lotto;
using namespace bts::lotto::helper;

#define LOTTO_TEST_NUM_WALLET_ADDRS   10
#define LOTTO_TEST_BLOCK_SECS         (5 * 60)

#define LOTTO_TEST_TRRANSFER_AMOUT_FOR_BLOCK             asset(uint64_t(1))

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

trx_block generate_genesis_block(const std::vector<address>& addr)
{
    trx_block genesis;
    genesis.version = 0;
    genesis.block_num = 0;
    genesis.timestamp = fc::time_point::now();
    genesis.next_fee = block_header::min_fee();
    genesis.total_shares = 0;

    /*
    signed_transaction secret_trx;
    secret_trx.vote = 0; // no vote
    secret_trx.outputs.push_back(trx_output(claim_secret_output(), asset()));
    genesis.trxs.push_back(secret_trx);
    */

    signed_transaction dtrx;
    dtrx.vote = 0;
    // create initial delegates
    for (uint32_t i = 0; i < 100; ++i)
    {
        auto name = "delegate-" + fc::to_string(int64_t(i + 1));
        auto key_hash = fc::sha256::hash(name.c_str(), name.size());
        auto key = fc::ecc::private_key::regenerate(key_hash);
        dtrx.outputs.push_back(trx_output(claim_name_output(name, std::string(), i + 1, key.get_public_key(), key.get_public_key()), asset()));
    }
    genesis.trxs.push_back(dtrx);

    // generate an initial genesis block that evenly allocates votes among all
    // delegates.
    for (uint32_t i = 0; i < 100; ++i)
    {
        signed_transaction trx;
        trx.vote = i + 1;
        for (uint32_t o = 0; o < 20; ++o)
        {
            uint64_t amnt = 100000;
            trx.outputs.push_back(trx_output(claim_by_signature_output(addr[i]), asset(amnt)));
            genesis.total_shares += amnt;
        }
        genesis.trxs.push_back(trx);
    }

    genesis.trx_mroot = genesis.calculate_merkle_root(signed_transactions());

    return genesis;
}

/* State for simulating a Lotto chain, with 2 wallets, and mining*/
class LottoTestState
{
    public:
        std::shared_ptr<sim_pow_validator>  validator;
        fc::ecc::private_key                auth;
        fc::path                            path;
        bts::lotto::lotto_db                db;

        bts::lotto::lotto_wallet            wallet1;
        bts::lotto::lotto_wallet            wallet2;

        std::vector<address>                addrs1;
        std::vector<address>                addrs2;

        LottoTestState()
        {
            validator = std::make_shared<sim_pow_validator>(fc::time_point::now());
            auth = fc::ecc::private_key::generate();
            fc::temp_directory dir;
            path = dir.path();

            db.set_trustee(auth.get_public_key());
            db.set_pow_validator(validator);
            db.open(path / "lotto_test_db", true);

            wallet1.create_internal(path / "lotto_test_wallet1.dat", "password", "password", true);
            wallet2.create_internal(path / "lotto_test_wallet2.dat", "password", "password", true);

            addrs1 = std::vector<address>();
            addrs2 = std::vector<address>();

            /* Start the blockchain with random balances in new addresses */
            for (auto i = 0; i < LOTTO_TEST_NUM_WALLET_ADDRS; ++i)
            {
                addrs1.push_back(wallet1.new_receive_address());
                addrs2.push_back(wallet2.new_receive_address());
            }

            // wallet1 as trustee
            wallet1.import_key(auth);

            std::vector<address> addrs = addrs1;
            addrs.insert(addrs.end(), addrs2.begin(), addrs2.end());
            auto genblk = generate_genesis_block(addrs);
            wlog("Start sign block.");
            genblk.sign(auth);
            wlog("Start push block.\n");
            db.push_block(genblk);
            wlog("Start wallet1 scan.\n");
            wallet1.scan_chain(db);
            wlog("Start wallet2 scan\n");
            wallet2.scan_chain(db);
        }

        ~LottoTestState()
        {
            delegate_secret_last_revealed_secret_pair(0);
            db.close();
            fc::remove_all(path);
        }

        /* Put these transactions into a block */
        void next_block(lotto_wallet &wallet, signed_transactions &txs)
        {
            wlog("next_block transactions: ${c}", ("c", txs));
            wlog("Current head block number is ${n}", ("n", db.head_block_num()));
            validator->skip_time(fc::seconds(LOTTO_TEST_BLOCK_SECS));

            if (txs.size() <= 0)
                txs.push_back(wallet.send_to_address(LOTTO_TEST_TRRANSFER_AMOUT_FOR_BLOCK, random_addr(wallet)));

            auto next_block = wallet.generate_next_block(db, txs);
            wlog("Start sign block.");
            next_block.sign(auth);
            wlog("Start push block.\n");
            db.push_block(next_block);
            wlog("Start wallet1 scan.\n");
            wallet1.scan_chain(db);
            wlog("Start wallet2 scan\n");
            wallet2.scan_chain(db);
            txs.clear();
        }

        void next_block(signed_transactions &txs)
        {
            signed_transaction secret_trx;
            claim_secret_output secret_out;

            // TODO: delegate id should start 1, 0 is reserved for genesis block secert
            secret_out.delegate_id = db.head_block_num() % 100 + 1;
            auto secret_pair = delegate_secret_last_revealed_secret_pair(secret_out.delegate_id);
            
            // hash of secret for this round
            secret_out.secret = secret_pair.first;
            // reveal secret of last round
            secret_out.revealed_secret = secret_pair.second;
            secret_trx.outputs.push_back(trx_output(secret_out, asset()));

            // TODO: requireing delegate's signature;
            std::unordered_set<address> required_signatures;
            required_signatures.insert(address(auth.get_public_key()));
            wallet1.collect_inputs_and_sign(secret_trx, asset(0), required_signatures, "secret transaction");

            wlog("secret_trx: ${tx} ", ("tx", secret_trx));

            txs.insert(txs.begin(), secret_trx);
            next_block(wallet1, txs);
        }

        /* Get a random existing address. Good for avoiding dust in certain tests. */
        address random_addr(lotto_wallet &wallet)
        {
            if (&wallet == &wallet1)
                return addrs1[rand() % (addrs1.size())];
            else if (&wallet == &wallet2)
                return addrs2[rand() % (addrs2.size())];

            throw;
        }

        address random_addr()
        {
            return random_addr(wallet1);
        }
};

BOOST_AUTO_TEST_CASE(trx_validator_claim_secret)
{
    try
    {
        LottoTestState state;
        lotto_transaction_validator validator(&state.db);

        signed_transaction signed_trx;

        /* Build secret output */
        claim_secret_output secret_out;
        secret_out.secret = fc::sha256::hash(fc::sha256("FFA12345699999999999"));
        secret_out.revealed_secret = fc::sha256("FFA12345699999999999");

        // amount must be zero
        signed_trx.outputs.push_back(trx_output(secret_out, asset()));

        // no inputs and no need to sign
        //validator.evaluate(signed_trx, validator.create_block_state());
    }
    catch (const fc::exception& e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(trx_validator_buy_ticket)
{
    try
    {
        LottoTestState state;
        lotto_transaction_validator validator(&state.db);

        auto signed_trx = state.wallet1.buy_ticket(88, 1, asset(100));
        wlog("tx: ${tx} ", ("tx", signed_trx));

        validator.evaluate(signed_trx, validator.create_block_state());
    }
    catch (const fc::exception &e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
}

/* lucky number is just a special buy_ticket trx that only buy 1 amout of ticket*/
BOOST_AUTO_TEST_CASE(trx_validator_lucky_number)
{
    try
    {
        LottoTestState state;
        lotto_transaction_validator validator(&state.db);

        auto signed_trx = state.wallet1.buy_ticket(888, 1, asset(100));
        wlog("tx: ${tx} ", ("tx", signed_trx));

        validator.evaluate(signed_trx, validator.create_block_state());
    }
    catch (const fc::exception& e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(wallet_list_tickets)
{
    try
    {
        LottoTestState state;

        signed_transactions txs;
        auto signed_trx = state.wallet1.buy_ticket(888, 1, asset(100));
        wlog("tx: ${tx} ", ("tx", signed_trx));

        txs.push_back(signed_trx);

        state.next_block(txs);

        auto tickets = state.wallet1.list_tickets(state.db);

        for (auto ticket : tickets)
        {
            wlog("out_idx: ${out_idx} ", ("out_idx", ticket.first));
            BOOST_CHECK(ticket.first.block_idx == 1);
            BOOST_CHECK(ticket.first.trx_idx == 1);
            auto ticket_out = ticket.second.as<claim_ticket_output>();
            BOOST_CHECK(ticket.second.amount == asset(100));
            auto t = ticket_out.ticket.as<betting_ticket>();
            BOOST_CHECK(t.lucky_number == 888);
            BOOST_CHECK(t.odds == 1);
        }
    }
    catch (const fc::exception& e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
    catch (const std::exception& e)
    {
        elog("${e}", ("e", e.what()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(wallet_list_jackpots)
{
    try
    {
        LottoTestState state;

        // Generate 101 blocks, and wait for secerets out...
        for (size_t i = 0; i < 102; i++)
        {
            signed_transactions txs;
            auto signed_trx = state.wallet1.buy_ticket(888, 1, asset(100));
            wlog("tx: ${tx} ", ("tx", signed_trx));

            txs.push_back(signed_trx);

            state.next_block(txs);
        }

        auto jackpots = state.wallet1.list_jackpots(state.db);

        wlog("jackpots count: ${c} ", ("c", jackpots.size()));
        BOOST_CHECK(jackpots.size() > 0);

        for (auto jackpot : jackpots)
        {
            wlog("out_idx: ${out_idx} ", ("out_idx", jackpot.first));
            wlog("jackpot: ${jackpot} ", ("jackpot", jackpot.second));
        }
    }
    catch (const fc::exception& e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
    catch (const std::exception& e)
    {
        elog("${e}", ("e", e.what()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(wallet_cash_jackpot)
{
    try
    {
        LottoTestState state;
        lotto_transaction_validator validator(&state.db);

        // Generate 101 blocks, and wait for secerets out...
        for (size_t i = 0; i < 102; i++)
        {
            signed_transactions txs;
            auto signed_trx = state.wallet1.buy_ticket(888, 1, asset(10000));
            wlog("tx: ${tx} ", ("tx", signed_trx));

            txs.push_back(signed_trx);

            state.next_block(txs);
        }

        auto jackpots = state.wallet1.list_jackpots(state.db);

        wlog("jackpots count: ${c} ", ("c", jackpots.size()));
        BOOST_CHECK(jackpots.size() > 0);

        asset old_balance = state.wallet1.get_balance(0);
        wlog("Current balance is: ${b}", ("b", old_balance));

        signed_transactions trxs;
        for (auto jackpot : jackpots)
        {
            auto trx = state.wallet1.cash_jackpot(jackpot.first);
            wlog("cash_jackpot tx: ${tx} ", ("tx", trx));
            validator.evaluate(trx, validator.create_block_state());
            trxs.push_back(trx);
        }

        state.next_block(trxs);

        asset new_balance = state.wallet1.get_balance(0);
        wlog("Current balance is: ${b}", ("b", state.wallet1.get_balance(0)));
        BOOST_CHECK(new_balance > old_balance);
    }

    catch (const fc::exception& e)
    {
        std::cerr << e.to_detail_string() << "\n";
        elog("${e}", ("e", e.to_detail_string()));
        throw;
    }
    catch (const std::exception& e)
    {
        elog("${e}", ("e", e.what()));
        throw;
    }
}