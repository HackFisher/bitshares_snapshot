#pragma once
#include <bts/rpc/rpc_server.hpp>

namespace bts { namespace lotto {
  using namespace bts::rpc;
  
  namespace detail
  {
    class lotto_rpc_server_impl;
  }
  
  class lotto_rpc_server : public bts::rpc::rpc_server
  {
  public:
    lotto_rpc_server();
    ~lotto_rpc_server();
  private:
    std::unique_ptr<detail::lotto_rpc_server_impl> my;
  };
  typedef std::shared_ptr<lotto_rpc_server> lotto_rpc_server_ptr;
} } // bts::lotto
