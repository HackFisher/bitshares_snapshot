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
} } // bts::lotto

