#pragma once

/*
 * The blocks number between ticket purchase block and winning number out block
 */
#define BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW 100

#define BTS_LOTTO_MAX_BALL_COUNT 256


// TO be move to rule config file

/*
 * The blocks number between ticket purchase block and winning number out block
 * Jackpot outputs in the same draw transanctions must have different mature days.
 * TODO: This config is to be moved to rule definition, checked by jackpot output.
 */
#define BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT 100000000000
