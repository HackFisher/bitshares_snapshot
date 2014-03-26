#pragma once
#include <fc/array.hpp>
#include <string>
#include <fc/uint128.hpp>
#include <fc/io/enum_type.hpp>
#include <stdint.h>
#include <bitset>

namespace bts { namespace lotto {

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
   class combination
   {
	  public:
	   combination();
       combination(uint32_t r, uint32_t b);
       combination(const std::vector<uint16_t>& nums);   // 5 red balls and 2 blue balls
	   combination(uint32_t num);

	   // representing combination using nature numbers, 0, 1, 2, ....
	   static uint32_t ranking(const std::vector<uint16_t>& combination);
	   // convert nature numbers to combination binary
       static std::vector<uint16_t> unranking(uint32_t num, uint16_t k, uint16_t n);

	   uint32_t to_num() const;
       
	   uint32_t red;
       uint32_t blue;
   };
   
} } // namespace bts::lotto


/* TODO: TBD
namespace fc 
{ 
   void to_variant( const bts::lotto::combination& var,  fc::variant& vo );
   void from_variant( const fc::variant& var,  bts::lotto::combination& vo );
}
*/

#include <fc/reflect/reflect.hpp>
FC_REFLECT( bts::lotto::combination, (red)(blue) )