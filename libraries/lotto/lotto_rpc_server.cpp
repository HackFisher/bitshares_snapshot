#include <bts/lotto/lotto_rpc_server.hpp>
#include <bts/lotto/lotto_wallet.hpp>
#include <boost/bind.hpp>

namespace bts { namespace lotto {

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

      fc::variant buy_ticket(const fc::variants& params);
      fc::variant lucky_number(const fc::variants& params);
      fc::variant draw_ticket(const fc::variants& params);
      fc::variant query_jackpots(const fc::variants& params);
    };

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
    fc::variant lotto_rpc_server_impl::lucky_number(const fc::variants& params)
    {
      FC_ASSERT(params.size() == 1);
      uint64_t lucky_number = params[0].as_uint64();
      asset amount(1.0);

      auto tx = get_lotto_wallet()->buy_ticket(lucky_number, 1, amount);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }
    fc::variant lotto_rpc_server_impl::draw_ticket(const fc::variants& params)
    {
      FC_ASSERT(params.size() == 1);
      uint64_t block_num = params[0].as_uint64();

      auto tx = get_lotto_wallet()->draw_ticket(*get_lotto_db(), block_num);

      _self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }
    fc::variant lotto_rpc_server_impl::query_jackpots(const fc::variants& params)
    {
      FC_ASSERT(params.size() == 2); // cmd name path
      std::string name = params[0].as_string();
      asset bid = params[1].as<asset>();
      signed_transactions tx_pool;

      //auto tx = get_lotto_wallet()->qui

      //_self->get_client()->broadcast_transaction(tx);
      return fc::variant(true);
    }

  } // end namespace detail

  lotto_rpc_server::lotto_rpc_server() :
    my(new detail::lotto_rpc_server_impl)
  {
    my->_self = this;

#define JSON_METHOD_IMPL(METHODNAME) \
    boost::bind(&detail::lotto_rpc_server_impl::METHODNAME, my.get(), _1)
      
    method_data buy_ticket_metadata{"buy_ticket", JSON_METHOD_IMPL(buy_ticket),
          /* description */ "buy tickets for specific lucky number with some odds",
          /* returns: */    "bool",
          /* params:          name            type       required */
                        {{"lucky_number",  "uint64_t",  true},
                         {"odds",          "uint64_t",  true},
                         {"amount",        "asset",     true}},
          /* prerequisites */ json_authenticated | wallet_open | wallet_unlocked};
    register_method(buy_ticket_metadata);
    
    method_data lucky_number_metadata{"lucky_number", JSON_METHOD_IMPL(lucky_number),
          /* description */ "buy 1 share of lotto ticket. FORMAT: R1 R2 R3 R4 R5 | B1 B2 e.g. 3 6 21 25 31 | 4 7",
          /* returns: */    "bool",
          /* params:          name            type       required */
                            {{"lucky_number",  "uint64_t",  true}},
          /* prerequisites */ json_authenticated | wallet_open | wallet_unlocked};
    register_method(lucky_number_metadata);
      
    method_data draw_ticket_metadata{"draw_ticket", JSON_METHOD_IMPL(lucky_number),
          /* description */ "TODO: draw tickets",
          /* returns: */    "bool",
          /* params:          name            type       required */
                            {{"block_number",  "uint64_t",  true}},
          /* prerequisites */ json_authenticated | wallet_open | wallet_unlocked};
    register_method(draw_ticket_metadata);
      
    method_data query_jackpots_metadata{"query_jackpots", JSON_METHOD_IMPL(lucky_number),
          /* description */ "TODO: ",
          /* returns: */    "bool",
          /* params:          name            type       required */
          {},
          /* prerequisites */ json_authenticated | wallet_open | wallet_unlocked};
    register_method(query_jackpots_metadata);

#undef JSON_METHOD_IMPL
  }

  lotto_rpc_server::~lotto_rpc_server()
  {
  }

} } // bts::lotto