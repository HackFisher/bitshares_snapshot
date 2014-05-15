#include <bts/lotto/lotto_operations.hpp>
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
    rule::rule(lotto_db* db, ticket_type t, asset_id_type u)
        :my(new detail::rule_impl())
    {
        _lotto_db = db;
        _ticket_type = t;
        _unit_type = u;
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

    /*
    void rule::validate(const full_block& blk, const signed_transactions& deterministic_trxs)
    {

    }

    void rule::store(const full_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state)
    {
    }*/
} } // bts::lotto

