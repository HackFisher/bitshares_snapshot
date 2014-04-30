#include <bts/lotto/lotto_rpc_server.hpp>
#include <bts/lotto/lotto_wallet.hpp>
#include <boost/bind.hpp>

namespace bts { namespace lotto {

#define LOTTO_RPC_METHOD_LIST\
                (buy_ticket)\
                (lucky_number)\
                (draw_ticket)\
                (query_jackpots)



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
      FC_ASSERT(params.size() == 3);
      uint64_t lucky_number = params[0].as_uint64();
      uint64_t odds = params[1].as_uint64();
      asset amount = params[2].as<asset>();
      signed_transactions tx_pool;

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
      FC_ASSERT(params.size() == 1);
      uint64_t lucky_number = params[0].as_uint64();
      asset amount(1.0);

      auto tx = get_lotto_wallet()->buy_ticket(lucky_number, 1, amount);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

    /*--------------------------draw_ticket--------------------*/
    static rpc_server::method_data draw_ticket_metadata{ "draw_ticket", nullptr,
        /* description */  "TODO: draw tickets.",
        /* returns: */    "bool",
        /* params:          name                 type      required */
                        { { "ticket_number", "uint64_t", true } },
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open | rpc_server::wallet_unlocked,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::draw_ticket(const fc::variants& params)
    {
      FC_ASSERT(params.size() == 1);
      uint64_t ticket_num_param = params[0].as_uint64();

      // TODO: To be tested. to convert to ticket_number(uint64_t) and ticket_number::as_uint64()
      ticket_number ticket_num(
          ticket_num_param >> 32, (ticket_num_param << 32) >> 48, (ticket_num_param << 48) >> 48);

      auto tx = get_lotto_wallet()->draw_ticket(*get_lotto_db(), ticket_num);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

    /*--------------------------query_jackpots--------------------*/
    static rpc_server::method_data query_jackpots_metadata{ "query_jackpots", nullptr,
        /* description */  "TODO:",
        /* returns: */    "bool",
        /* params:          name                 type      required */
        {},
        /* prerequisites */ rpc_server::json_authenticated | rpc_server::wallet_open | rpc_server::wallet_unlocked,
        R"(
TODO:
    )" };
    fc::variant lotto_rpc_server_impl::query_jackpots(const fc::variants& params)
    {
      FC_ASSERT(params.size() == 2); // cmd name path
      std::string name = params[0].as_string();
      asset bid = params[1].as<asset>();
      signed_transactions tx_pool;

      // TODO
      //auto tx = get_lotto_wallet()->qui

      //_self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

    // TODO: query tickets, also show the related jackpots.

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