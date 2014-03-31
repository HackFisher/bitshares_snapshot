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
    typedef std::vector<uint8_t> combination;
    /* Ranking groups
     * 
     */
    typedef std::vector<uint64_t> c_rankings;
    typedef std::vector<uint8_t> match;

    /*  Ball counts uint8_t
     *  
     */
    struct rule_config
    {
        rule_config():valid(false),version(0),id(0), name(""){}
		bool											valid;
        uint16_t                                        version;
        uint32_t                                        id;
        fc::string                                      name;
        std::vector< std::pair<uint8_t,uint8_t> >     balls;
        std::vector< /* levels */ std::pair<
            /*level*/ uint8_t, /* matches */ std::vector<match> > 
            > prizes;
    };

    

    const rule_config& global_rule_config();

    uint8_t GROUP_COUNT();

	const std::vector<uint64_t>& GROUP_SPACES();

	uint64_t TOTAL_SPACE();

namespace bts { namespace lotto {
	// calculate combinational number, TODO: inline?
	uint64_t Combination(uint8_t N, uint8_t k);

    // ranking of multi indepent nature numbers C(N1,k1) * C(N2,k2) * C(N3,k3)
	// nums in c_ranking are start from 0,spaces are the total counts for each ranking
	uint64_t ranking(const c_rankings& r, const std::vector<uint64_t>& spaces );

	// unranking to combination rankings
	// minimum numbers in returen value should start from 0, maximum is (N-1), N is space
	std::shared_ptr<c_rankings> unranking(uint64_t num, const std::vector<uint64_t>& spaces );

    // representing combination using nature numbers, 0, 1, 2, ....
	uint64_t ranking(const combination& c);
	
    // convert nature numbers to combination binary
    std::shared_ptr<combination> unranking(uint64_t num, uint8_t k, uint8_t n);
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