#pragma once

#include <bts/blockchain/address.hpp>
#include <bts/blockchain/pts_address.hpp>
#include <bts/blockchain/asset.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/io/varint.hpp>

namespace bts { namespace lotto {

    /**
     *  @enum ticket_type_enum
     *  @brief Enumerates the types of supported ticket types
     */
    enum ticket_type_enum
    {
        null_ticket_type = 0,
        ticket_for_betting = 1,
        ticket_for_lottery = 2
    };

    typedef uint8_t ticket_type;

    struct betting_ticket
    {
        static const ticket_type_enum type;
        betting_ticket(const uint64_t& l, const uint16_t& o)
            :lucky_number(l), odds(o){}
        betting_ticket(){}
        /**
         *  This is the number chosen by the user or at
         *  random, ie: their lotto ticket number.
         */
        // TODO: to be deleted betting do not need this field
        uint64_t                   lucky_number;

        /** The probability of winning... increasing the odds will
         * cause the amount won to grow by Jackpot * odds, but the
         * probability of winning decreases by 2*odds.
         */
        uint16_t                   odds;
    };

    struct lottery_ticket
    {
        static const ticket_type_enum type;
        lottery_ticket(const uint64_t& l)
            :lucky_number(l){}
        lottery_ticket(){}
        /**
         *  This is the number chosen by the user or at
         *  random, ie: their lotto ticket number.
         */
        // TODO: to be deleted betting do not need this field
        uint64_t                   lucky_number;
    };

    struct output_ticket
    {
        template<typename TicketType>
        output_ticket(const TicketType& t)
        {
            ticket_func = TicketType::type;
            ticket_data = fc::raw::pack(t);
        }

        template<typename TicketType>
        TicketType as()const
        {
            FC_ASSERT(ticket_func == TicketType::type, "", ("ticket_func", ticket_func)("TicketType", TicketType::type));
            return fc::raw::unpack<TicketType>(ticket_data);
        }

        output_ticket(){}
        
        ticket_type                                 ticket_func;
        std::vector<char>                           ticket_data;
    };
} } // bts::lotto

namespace fc {
    void to_variant(const bts::lotto::output_ticket& var, variant& vo);
    void from_variant(const variant& var, bts::lotto::output_ticket& vo);
};

FC_REFLECT_ENUM(bts::lotto::ticket_type_enum,
    (null_ticket_type)
    (ticket_for_betting)
    (ticket_for_lottery)
    )
FC_REFLECT(bts::lotto::betting_ticket, (lucky_number)(odds))
FC_REFLECT(bts::lotto::lottery_ticket, (lucky_number))
FC_REFLECT(bts::lotto::output_ticket, (ticket_func)(ticket_data))