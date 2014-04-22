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
#include <bts/lotto/lotto_db.hpp>

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
    typedef std::vector<uint64_t>						c_rankings;
    typedef std::vector<uint8_t>						match;
	typedef std::vector< std::pair<uint8_t,uint8_t> >	type_balls;
	typedef std::vector< 
		std::pair</*level*/ uint8_t, 
		/* matches */ std::vector<match>> >				type_prizes;

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
        type_balls     balls;
        type_prizes prizes;
    };

    

    const rule_config& global_rule_config();

    uint8_t GROUP_COUNT();

	const std::vector<uint64_t>& GROUP_SPACES();

	uint64_t TOTAL_SPACE();


namespace bts { namespace lotto {
	/**
     * Calculate combinational number, TODO: inline?
     * @return C(N, k)
     */
	uint64_t Combination(uint8_t N, uint8_t k);

    /**
     * Match different combination groups according to group type (N, k)
     * @return the vector of matched count as elements.
     */
	match match_rankings(const c_rankings& l, const c_rankings& r, const type_balls& balls);

    /**
     * Ranking of multi group combination ranked numbers C(N1,k1) * C(N2,k2) * C(N3,k3)
     * Numbers in c_ranking start from 0, and spaces are the group space size for each group
     * @return the ranking of multi combination groups
	 */ 
	uint64_t ranking(const c_rankings& r, const std::vector<uint64_t>& spaces );

	/*
     * Unranking to combination groups' ranking, this is the reverse process of ranking
	 * Numbers in returen value should start from 0, maximum is (N-1), N is group space size
     * @return combination ranked numbers C(N1,k1) * C(N2,k2) * C(N3,k3)
     */
	c_rankings unranking(uint64_t num, const std::vector<uint64_t>& spaces );

    /* 
     * Parameters is combination of a group of nature numbers, number in this group is from 0 to N - 1 , N is the group count
     * Convert group combination to continous nature nubmers from 0 to C(N, k) - 1, using algorithm combinatorial number system
     * @return ranking value of the combination
     */
	uint64_t ranking(const combination& c);
	
    /*
     * Convert nature numbers to combination binary
     * Reverse process of ranking
     */
    combination unranking(uint64_t num, uint8_t k, uint8_t n);

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