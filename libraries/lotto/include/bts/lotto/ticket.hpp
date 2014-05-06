#pragma once

#include <bts/blockchain/address.hpp>
#include <bts/blockchain/pts_address.hpp>
#include <bts/blockchain/asset.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
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

    struct ticket_for_betting_data
    {
        static const ticket_type_enum type;
        ticket_for_betting_data(const uint64_t& l, const uint64_t& o) 
            :lucky_number(l), odds(o){}
        ticket_for_betting_data(){}
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

    struct ticket_for_lottery_data
    {
        static const ticket_type_enum type;
        ticket_for_lottery_data(const uint64_t& l)
            :lucky_number(l){}
        ticket_for_lottery_data(){}
        /**
         *  This is the number chosen by the user or at
         *  random, ie: their lotto ticket number.
         */
        // TODO: to be deleted betting do not need this field
        uint64_t                   lucky_number;
    };

    struct ticket_data
    {
        template<typename TicketType>
        ticket_data(const TicketType& t, const bts::blockchain::address& a)
            :owner(a)
        {
            claim_func = TicketType::type;
            claim_data = fc::raw::pack(t);
        }

        template<typename TicketType>
        TicketType as()const
        {
            FC_ASSERT(claim_func == TicketType::type, "", ("ticket_func", ticket_func)("TicketType", TicketType::type));
            return fc::raw::unpack<TicketType>(data);
        }

        ticket_data(){}

        bts::blockchain::address                    owner;
        ticket_type                                 ticket_func;
        std::vector<char>                           data;
    };
} } // bts::lotto

FC_REFLECT_ENUM(bts::lotto::ticket_type_enum,
    (null_ticket_type)
    (ticket_for_betting)
    (ticket_for_lottery)
)

namespace fc {
    void to_variant(const bts::lotto::ticket_data& var, variant& vo);
    void from_variant(const variant& var, bts::lotto::ticket_data& vo);
};

FC_REFLECT(bts::lotto::ticket_for_betting_data, (lucky_number)(odds))
FC_REFLECT(bts::lotto::ticket_for_lottery_data, (lucky_number))
FC_REFLECT(bts::lotto::ticket_data, (owner)(ticket_func)(data))