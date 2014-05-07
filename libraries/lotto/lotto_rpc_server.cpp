#include <boost/bind.hpp>

#include <fc/log/logger.hpp>

#include <bts/lotto/lotto_rpc_server.hpp>
#include <bts/lotto/lotto_wallet.hpp>

namespace bts { namespace lotto {

#define LOTTO_RPC_METHOD_LIST\
                (buy_ticket)\
                (lucky_number)\
                (list_tickets)\
                (list_jackpots)\
                (cash_jackpot)



  namespace detail
  {
    class lotto_rpc_server_impl
    {
    public:
      lotto_rpc_server* _self;

      const lotto_wallet_ptr get_lotto_wallet()
      {
        return std::dynamic_pointer_cast<lotto_wallet>(_self->get_client()->get_wallet());
      }
      const lotto_db_ptr get_lotto_db()
      {
        return std::dynamic_pointer_cast<lotto_db>(_self->get_client()->get_chain());
      }

#define DECLARE_RPC_METHOD( r, visitor, elem )  fc::variant elem( const fc::variants& );
#define DECLARE_RPC_METHODS( METHODS ) BOOST_PP_SEQ_FOR_EACH( DECLARE_RPC_METHOD, v, METHODS ) 
        DECLARE_RPC_METHODS( LOTTO_RPC_METHOD_LIST )
#undef DECLARE_RPC_METHOD
#undef DECLARE_RPC_METHODS
    };

    /*--------------------------buy_ticket--------------------*/
    static rpc_server::method_data buy_ticket_metadata{ "buy_ticket", nullptr,
        /* description */  "TODO:buy tickets for specific lucky number with some odds.",
        /* returns: */    "bool",
        /* params:          name                 type      required */
                        { { "lucky_number", "uint64_t", true },
                          { "odds", "uint64_t", true },
                          { "amount", "asset", true } },
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open | rpc_server::wallet_unlocked,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::buy_ticket(const fc::variants& params)
    {
        // TODO: do more interactive cli and check with the inputs...
      FC_ASSERT(params.size() == 3);
      uint64_t lucky_number = params[0].as_uint64();
      uint16_t odds = params[1].as<uint16_t>();
      asset amount = params[2].as<asset>();

      auto tx = get_lotto_wallet()->buy_ticket(lucky_number, odds, amount);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

    /*--------------------------lucky_number--------------------*/
    static rpc_server::method_data lucky_number_metadata{ "lucky_number", nullptr,
        /* description */  "TODO:buy 1 share of lotto ticket.",
        /* returns: */    "bool",
        /* params:          name                 type      required */
                            { { "lucky_number", "uint64_t", true } },
         /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open | rpc_server::wallet_unlocked,
        R"(
TODO: FORMAT: R1 R2 R3 R4 R5 | B1 B2 e.g. 3 6 21 25 31 | 4 7
    )" };
    fc::variant lotto_rpc_server_impl::lucky_number(const fc::variants& params)
    {
        // TODO: do more interactive cli and check with the inputs...
      FC_ASSERT(params.size() == 1);
      uint64_t lucky_number = params[0].as_uint64();
      asset amount(1);

      auto tx = get_lotto_wallet()->buy_ticket(lucky_number, 1, amount);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

    /*--------------------------list_tickets--------------------*/
    static rpc_server::method_data list_tickets_metadata{ "list_tickets", nullptr,
        /* description */  "TODO: list tickets.",
        /* returns: */    "map<output_index, trx_output>",
        /* params:          name                 type      required */
                        {},
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::list_tickets(const fc::variants& params)
    {
        try
        {
            auto tickets = get_lotto_wallet()->list_tickets(*get_lotto_db());
            return fc::variant(tickets);
        }
        catch (const fc::exception& e)
        {
            wlog("${e}", ("e", e.to_detail_string()));
            throw;
        }
        catch (...)
        {
            throw rpc_wallet_passphrase_incorrect_exception();
        }
    }

    /*--------------------------list_jackpots--------------------*/
    static rpc_server::method_data list_jackpots_metadata{ "list_jackpots", nullptr,
        /* description */  "TODO:",
        /* returns: */    "map<output_index, trx_output>",
        /* params:          name                 type      required */
        {},
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::list_jackpots(const fc::variants& params)
    {
        try
        {
            auto jackpots = get_lotto_wallet()->list_jackpots(*get_lotto_db());
            return fc::variant(jackpots);
        }
        catch (const fc::exception& e)
        {
            wlog("${e}", ("e", e.to_detail_string()));
            throw;
        }
        catch (...)
        {
            throw rpc_wallet_passphrase_incorrect_exception();
        }
    }

    /*--------------------------cash_jackpot--------------------*/
    // TODO: replace parameter with uint64_t?
    static rpc_server::method_data cash_jackpot_metadata{ "cash_jackpot", nullptr,
        /* description */  "Cash the jackpot to balance",
        /* returns: */    "bool",
        /* params:          name                 type      required */
        { { "jackpot_idx", "output_idx", true } },
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open | rpc_server::wallet_unlocked,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::cash_jackpot(const fc::variants& params)
    {
        FC_ASSERT(params.size() >= 1);
        auto jackpot_idx = params[0].as<output_index>();

        auto tx = get_lotto_wallet()->cash_jackpot(jackpot_idx);

        _self->get_client()->broadcast_transaction(tx);
        return fc::variant(true);
    }

  } // end namespace detail

  lotto_rpc_server::lotto_rpc_server() :
    my(new detail::lotto_rpc_server_impl)
  {
    my->_self = this;

#define REGISTER_LOTTO_RPC_METHOD( r, visitor, METHODNAME ) \
    do { \
        method_data data_with_functor(detail::BOOST_PP_CAT(METHODNAME,_metadata)); \
        data_with_functor.method = boost::bind(&detail::lotto_rpc_server_impl::METHODNAME, my.get(), _1); \
        register_method(data_with_functor); \
    } while (0);
#define REGISTER_LOTTO_RPC_METHODS( METHODS ) \
    BOOST_PP_SEQ_FOR_EACH( REGISTER_LOTTO_RPC_METHOD, v, METHODS ) 

    REGISTER_LOTTO_RPC_METHODS( LOTTO_RPC_METHOD_LIST )
                        
#undef REGISTER_LOTTO_RPC_METHOD
#undef REGISTER_LOTTO_RPC_METHODS
  }

  lotto_rpc_server::~lotto_rpc_server()
  {
  }

} } // bts::lotto