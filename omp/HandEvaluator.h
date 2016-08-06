#ifndef OMPEVAL_H
#define OMPEVAL_H

#include "Util.h"
#include "Constants.h"
#include "Hand.h"
#include <cstdint>
#include <cassert>

namespace omp {

// Evaluates hands with any number of cards up to 7.
class HandEvaluator
{
public:
    HandEvaluator();

    // Returns the rank of a hand as a 16-bit integer. Higher value is better. Can also rank hands with less than 5
    // cards. A missing card is considered the worst kicker, e.g. K < KQJT8 < A < AK < KKAQJ < AA < AA2 < AA4 < AA432.
    // Hand category can be extracted by dividing the value by 4096. 1=highcard, 2=pair, etc.
    template<bool tFlushPossible = true>
    uint16_t evaluate(const Hand& hand) const
    {
        omp_assert(hand.count() <= 7 && hand.count() == bitCount(hand.mask()));
        // Hand has a 4-bit counter for each suit. It starts at 3 so the 4th bit gets set when there is 5 or more cards
        // of that suit.
        uint64_t flushCheck = hand.key() & Hand::FLUSH_CHECK_MASK;
        if (!tFlushPossible || !flushCheck) {
            // Get lookup key from the low 32 bits.
            unsigned key = (uint32_t)hand.key();
            return LOOKUP[perfHash(key)];
        } else {
            // Get the index of the flush check bit and use it to get the card mask for that suit.
            unsigned shift = countTrailingZeros((unsigned)(flushCheck >> 35)) << 2;
            unsigned flushKey = (uint16_t)(hand.mask() >> shift); // Get card mask of the correct suit.
            omp_assert(flushKey < FLUSH_LOOKUP_SIZE);
            return FLUSH_LOOKUP[flushKey];
        }
    }

private:
    static unsigned perfHash(uint32_t key)
    {
        omp_assert(key <= MAX_KEY);
        return key + PERF_HASH_ROW_OFFSETS[key >> PERF_HASH_ROW_SHIFT];
    }

    static bool cardInit;
    static void initCardConstants();
    static void staticInit();
    static void calculatePerfectHashOffsets();
    static unsigned populateLookup(uint64_t rankCounts, unsigned ncards, unsigned handValue, unsigned endRank,
                                   unsigned maxPair, unsigned maxTrips, unsigned maxStraight, bool flush = false);
    static unsigned getKey(uint64_t rankCounts, bool flush);
    static unsigned getBiggestStraight(uint64_t rankCounts);

    // Rank multipliers for non-flush and flush hands.
    static const unsigned RANKS[RANK_COUNT];
    static const unsigned FLUSH_RANKS[RANK_COUNT];

    // Turn on to recalculate and output the offset array.
    static const bool RECALCULATE_PERF_HASH_OFFSETS = false;

    // Determines in how many rows the original lookup table is divided (2^shift). More rows means slightly smaller
    // lookup table but much bigger offset table.
    static const unsigned PERF_HASH_ROW_SHIFT = 12;
    static const unsigned PERF_HASH_COLUMN_MASK = (1 << PERF_HASH_ROW_SHIFT) - 1;

    // Minimum number of cards required for evaluating a hand. Can be set to higher value to decrease lookup
    // table size (requires hash recalculation).
    static const unsigned MIN_CARDS = 0;

    // Lookup tables
    static const unsigned MAX_KEY;
    static const size_t FLUSH_LOOKUP_SIZE = 8192;
    static uint16_t* ORIG_LOOKUP;
    static uint16_t LOOKUP[86529 + RECALCULATE_PERF_HASH_OFFSETS * 100000000];
    static uint16_t FLUSH_LOOKUP[FLUSH_LOOKUP_SIZE];
    static uint32_t PERF_HASH_ROW_OFFSETS[8191 + RECALCULATE_PERF_HASH_OFFSETS * 100000];
};

}

#endif // OMPEVAL_H
