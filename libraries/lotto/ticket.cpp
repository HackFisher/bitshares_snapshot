#include <bts/lotto/ticket.hpp>
#include <bts/lotto/ticket_factory.hpp>
#include <fc/io/raw.hpp>

namespace bts {
    namespace lotto {
        const ticket_type_enum ticket_for_betting_data::type = ticket_type_enum::ticket_for_betting;
        const ticket_type_enum ticket_for_lottery_data::type = ticket_type_enum::ticket_for_lottery;

        ticket_factory& ticket_factory::instance()
        {
            static std::unique_ptr<ticket_factory> inst(new ticket_factory());
            return *inst;
        }

        void ticket_factory::to_variant(const ticket_data& in, fc::variant& output)
        {
            auto converter_itr = _converters.find(in.ticket_func);
            FC_ASSERT(converter_itr != _converters.end());
            converter_itr->second->to_variant(in, output);
        }
        void ticket_factory::from_variant(const fc::variant& in, ticket_data& output)
        {
            auto obj = in.get_object();
            output.ticket_func = obj["ticket_func"].as<ticket_type>();

            auto converter_itr = _converters.find(output.ticket_func);
            FC_ASSERT(converter_itr != _converters.end());
            converter_itr->second->from_variant(in, output);
        }
    }
} // bts::lotto

namespace fc {
    void to_variant(const bts::lotto::ticket_data& var, variant& vo)
    {
        bts::lotto::ticket_factory::instance().to_variant(var, vo);
    }

    /** @todo update this to use a factory and be polymorphic for derived blockchains */
    void from_variant(const variant& var, bts::lotto::ticket_data& vo)
    {
        bts::lotto::ticket_factory::instance().from_variant(var, vo);
    }
};