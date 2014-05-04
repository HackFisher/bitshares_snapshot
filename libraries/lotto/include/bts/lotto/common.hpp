#pragma once
#include <string>
#include <stdint.h>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <fc/array.hpp>
#include <fc/uint128.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>

#include <bts/blockchain/asset.hpp>

#include <bts/lotto/lotto_rule.hpp>

namespace bts { namespace lotto {
    const lotto_rule::config& global_rule_config();

    uint8_t GROUP_COUNT();

    const std::vector<uint64_t>& GROUP_SPACES();

    uint64_t TOTAL_SPACE();


	/**
     * Calculate combinational number, TODO: inline?
     * @return C(N, k)
     */
    uint64_t Combination(uint16_t N, uint16_t k);

    /**
     * Match different combination groups according to group type (N, k)
     * @return the vector of matched count as elements.
     */
    group_match match_rankings(const c_rankings& l, const c_rankings& r, const type_ball_group& balls);

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
    combination unranking(uint64_t num, uint16_t k, uint16_t n);

} } // namespace bts::lotto