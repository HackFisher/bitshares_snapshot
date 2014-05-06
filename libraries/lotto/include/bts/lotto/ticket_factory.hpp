#pragma once
#include <bts/lotto/ticket.hpp>

namespace bts {
    namespace lotto {

        class ticket_factory
        {
        public:
            static ticket_factory& instance();
            class ticket_converter_base
            {
            public:
                virtual ~ticket_converter_base(){};
                virtual void to_variant(const ticket_data& in, fc::variant& out) = 0;
                virtual void from_variant(const fc::variant& in, ticket_data& output) = 0;
            };

            template<typename TicketType>
            class ticket_converter : public ticket_converter_base
            {
            public:
                virtual void to_variant(const ticket_data& in, fc::variant& output)
                {
                    FC_ASSERT(in.ticket_func == TicketType::type);
                    fc::mutable_variant_object obj;

                    obj["owner"] = in.owner;
                    obj["ticket_func"] = in.ticket_func;
                    obj["data"] = fc::raw::unpack<TicketType>(in.data);

                    output = std::move(obj);
                }
                virtual void from_variant(const fc::variant& in, ticket_data& output)
                {
                    auto obj = in.get_object();
                    output.owner = obj["owner"].as<bts::blockchain::address>();

                    // already done once outside in ticket.cpp
                    output.ticket_func = obj["ticket_func"].as<ticket_type>();

                    FC_ASSERT(output.ticket_func == TicketType::type);
                    output.data = fc::raw::pack(obj["data"].as<TicketType>());
                }
            };

            template<typename TicketType>
            void   register_ticket()
            {
                _converters[TicketType::type] = std::make_shared< ticket_converter<TicketType> >();
            }

            /// defined in ticket.cpp
            void to_variant(const ticket_data& in, fc::variant& output);
            /// defined in ticket.cpp
            void from_variant(const fc::variant& in, ticket_data& output);
        private:

            std::unordered_map<int, std::shared_ptr<ticket_converter_base> > _converters;

        };

    }
} // bts::lotto
