#include <bts/lotto/lotto_outputs.hpp>
#include <fc/reflect/variant.hpp>
#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {
    namespace detail
    {
        class rule_impl
        {
        public:
            rule_impl(){}
        };
    }

    // Default rule validator implement, TODO: may be move default to another class
    rule::rule()
        :my(new detail::rule_impl())
    {
    }
    rule::~rule()
    {
    }

    void                  rule::open( const fc::path& dir, bool create)
    {
    }

    void                  rule::close()
    {
    }

    void rule::validate( const trx_block& blk, const signed_transactions& deterministic_trxs )
    {

    }

    void rule::store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state )
    {
    }
} } // bts::lotto

