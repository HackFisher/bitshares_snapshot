#pragma once
#include <fc/array.hpp>
#include <string>
#include <fc/uint128.hpp>
#include <fc/io/enum_type.hpp>
#include <stdint.h>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>

/**
    *  @brief encapsulates an combination number in
    *  integer form.   It can be converted to arrays or bits for matching
    *  winning or input purposes and can also be constructed from an combinatorial representation of integer
    *  according to Combinatorial number system
    *  TODO: decribe data infos and bytes etc.
	*  2. change to struct seems to be better, is one ticket value
	*  3. some function change to inline
	*  4. TODO: replace vectors and binary with BOOST_BINARY( 100 111000 01 1 110 )
	*    http://www.boost.org/doc/libs/1_42_0/libs/utility/utility.htm#BOOST_BINARY
    */
    typedef std::vector<uint16_t> combination;
    /* Ranking groups
     * 
     */
    typedef std::vector<uint32_t> rankings;
    typedef std::vector<uint16_t> match;

    /*  Ball counts uint16_t
     *  
     */
    struct rule_config
    {
        rule_config():version(0),id(0), name(""){}
        uint16_t                                        version;
        uint64_t                                        id;
        fc::string                                      name;
        std::vector< std::pair<uint16_t,uint16_t> >     balls;
        std::vector< /* levels */ std::pair<
            /*level*/ uint16_t, /* matches */ std::vector<match> > 
            > prizes;
    };

    

    rule_config load_rule_config();

    uint16_t rule_group_count();

namespace bts { namespace lotto {
    

    // representing combination using nature numbers, 0, 1, 2, ....
	uint32_t ranking(const combination& c);
	
    // convert nature numbers to combination binary
    std::shared_ptr<combination> unranking(uint32_t num, uint16_t k, uint16_t n);
} } // namespace bts::lotto


/* TODO: TBD
namespace fc 
{ 
   void to_variant( const bts::lotto::combination& var,  fc::variant& vo );
   void from_variant( const fc::variant& var,  bts::lotto::combination& vo );
}
*/

#include <fc/reflect/reflect.hpp>
FC_REFLECT( rule_config, (version)(id)(name)(balls)(prizes))