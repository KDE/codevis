// bjgc_dealertableutil.cpp                                           -*-C++-*-
#include <bjgc_dealertableutil.h>

#include <bjgc_dealertable.h>

#include <bjgb_dealercount.h>
#include <bjgb_shoe.h>
#include <bjgb_types.h> // 'Double'

#include <cstring> // 'memcpy()'
#include <iostream> // TBD TEMPORARY

// STATIC HELPER FUNCTIONS
static void f_h11_h16(bjgc::DealerTable *table, const bjgb::Shoe& shoe, bool dealerStandsOnSoft17Flag)
// Load, into the specified 'table' for rows hard(16) to hard(11), the
// probability of the dealer's getting the corresponding final (column)
// count given the (row) initial state:
//
// P(17, 16) is the probability of getting a hard(17) given a hard(16),
// which is exactly shoe.prob(1) * P(17, 17).
// P(18, 16) is the probability of getting a hard(18) given a hard(16),
// which is exactly shoe.prob(1) * P(18, 17) + shoe.prob(2) * P(18, 18).
// ...
// P(21, 16) is the probability of getting a hard(21) given a hard(16),
// which is exactly shoe.prob(1) * P(21, 17) + shoe.prob(2) * P(21, 18)
//                + shoe.prob(3) * P(21, 19) + shoe.prob(4) * P(21, 20)
//                + shoe.prob(5) * P(21, 21).
// P(BJ, 16) is the probability of getting a blackjack given a hard(16),
// which is exactly 0.0.
// P(OV, 16) is the probability of getting >= 22 given a hard(16),
// which is exactly 1.0 - the sum of the rest (17-21) above.
//
// P(17, 15) is the probability of getting a hard(17) given a hard(15),
// which is exactly shoe.prob(1) * P(17, 16) + shoe.prob(2) * P(17, 17).
// ...
// P(17, 12) is the probability of getting a hard(17) given a hard(12),
// which is exactly shoe.prob(1) * P(17, 13) + shoe.prob(2) * P(17, 14)
//                + shoe.prob(3) * P(17, 15) + shoe.prob(4) * P(17, 16)
//                + shoe.prob(5) * P(17, 17).
// ...
// P(21, 12) is the probability of getting a hard(21) given a hard(12),
// which is exactly shoe.prob(1) * P(21, 13) + shoe.prob(2) * P(21, 14)
//                + shoe.prob(3) * P(21, 15) + shoe.prob(4) * P(21, 16)
//                + shoe.prob(5) * P(21, 17) + shoe.prob(6) * P(21, 18)
//                + shoe.prob(7) * P(21, 19) + shoe.prob(8) * P(21, 20)
//                + shoe.prob(9) * P(21, 21).
// P(BJ, 12) is the probability of getting a blackjack given a hard(12),
// which is exactly 0.0.
// P(OV, 12) is the probability of getting >= 22 given a hard(12),
// which is exactly 1.0 - the sum of the rest (17-21) above.
//
// P(17, 11) is the probability of getting a hard(17) given a hard(11),
// which is exactly shoe.prob(1) * P(17, 12) + shoe.prob(2) * P(17, 13)
//                + shoe.prob(3) * P(17, 14) + shoe.prob(4) * P(17, 15)
//                + shoe.prob(5) * P(17, 16) + shoe.prob(6) * P(17, 17).
// ...
// P(21, 11) is the probability of getting a hard(21) given a hard(11),
// which is exactly shoe.prob(1) * P(21, 12) + shoe.prob(2) * P(21, 13)
//                + shoe.prob(3) * P(21, 14) + shoe.prob(4) * P(21, 15)
//                + shoe.prob(5) * P(21, 16) + shoe.prob(6) * P(21, 17)
//                + shoe.prob(7) * P(21, 18) + shoe.prob(8) * P(21, 19)
//                + shoe.prob(9) * P(21, 20) + shoe.prob(T) * P(21, 21)
//
// P(BJ, 11) is the probability of getting a blackjack given a hard(11),
// which is exactly 0.0.
// P(OV, 11) is the probability of getting >= 22 given a hard(11),
// which is exactly 1.0 - the sum of the rest (17-21) above.
//
// We need to accumulate the probability of winding up with each final
// value for the specified row in the dealer table.  We start with a
// count, 'j', of 16 and reduce it each time until we reach 11, where an
// Ace involves a soft count.
{
    for (int j = 16; j >= 11; --j) { // 'hard(j)' is the row we are filling.

        table->clearRow(bjgb::State::hard(j)); // Set all entries in row to 0
                                               // (including 'e_CBJ').

        for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
            // For each next card, up to but not including a busting card, add
            // each dealt card multiplied by its probability.

            for (int i = 1; i + j <= 21; ++i) { // for each final count <= 21
                                                // in column 'k'

                table->prob(bjgb::State::hard(j), bjgb::DealerCount::fini(k)) +=
                    shoe.prob(i) * table->prob(bjgb::State::hard(j + i), bjgb::DealerCount::fini(k));
            }
        }

        // Address column 'e_COV' separately.  'total' is used to calculate
        // "over" probability.  It could be done independently without
        // cumulative error, but that isn't significant.

        bjgb::Types::Double total = 0.0;

        for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
            total += table->prob(bjgb::State::hard(j), // accumulate the row
                                 bjgb::DealerCount::fini(k));
        }

        table->prob(bjgb::State::hard(j), // "over" is everything else.
                    bjgb::DealerCount::e_COV) = 1 - total;
    }
}

static void f_s1_s11(bjgc::DealerTable *table, const bjgb::Shoe& shoe, bool dealerStandsOnSoft17Flag)
// Load, into the specified 'table' for rows soft(1) to soft(11), the
// probability of the dealer's getting the corresponding final (column)
// count given the (row) initial state.
//
// Note that this calculation depends on whether the dealer hits on a soft
// 17, soft(7), for values soft(7) - soft(1).
//
// Dealer cannot hit a soft 21 [soft(11)]:
// P(17, S11) is the probability of getting a hard(17) given a soft(11),
// which is exactly 0.0.
// P(18, S11) is the probability of getting a hard(18) given a soft(11),
// which is exactly 0.0.
// P(19, S11) is the probability of getting a hard(19) given a soft(11),
// which is exactly 0.0.
// P(20, S11) is the probability of getting a hard(20) given a soft(11),
// which is exactly 0.0.
// P(21, S11) is the probability of getting a hard(21) given a soft(11),
// which is exactly 1.0.
//
// ...
//
// Dealer cannot hit a soft 18 [soft(8)]:
// P(17, S08) is the probability of getting a hard(17) given a soft(8),
// which is exactly 0.0.
// P(18, S08) is the probability of getting a hard(18) given a soft(8),
// which is exactly 1.0.
// P(19, S08) is the probability of getting a hard(19) given a soft(8),
// which is exactly 0.0.
// P(20, S08) is the probability of getting a hard(20) given a soft(8),
// which is exactly 0.0.
// P(21, S08) is the probability of getting a hard(21) given a soft(8),
// which is exactly 0.0.
//
// If dealer stands on soft 17:
// P(17, S07) is the probability of getting a hard(17) given a soft(7),
// which is exactly 1.0.
// P(18, S07) is the probability of getting a hard(18) given a soft(7),
// which is exactly 0.0.
// P(19, S07) is the probability of getting a hard(19) given a soft(7),
// which is exactly 0.0.
// P(20, S07) is the probability of getting a hard(20) given a soft(7),
// which is exactly 0.0.
// P(21, S07) is the probability of getting a hard(21) given a soft(7),
// which is exactly 0.0.
//
// If dealer hits on soft 17:
// P(17, S07) is the probability of getting a hard(17) given a soft(7),
// which is exactly shoe.prob( 5) * P(17, 12) + shoe.prob( 6) * P(17, 13)
//                + shoe.prob( 7) * P(17, 14) + shoe.prob( 8) * P(17, 15)
//                + shoe.prob( 9) * P(17, 16) + shoe.prob(10) * P(17, 17)
//                + shoe.prob( 1) * P(17,S08) + shoe.prob( 2) * P(17,S09)
//                + shoe.prob( 3) * P(17,S10) + shoe.prob( 4) * P(17,S11).
//
// P(18, S07) is the probability of getting a hard(18) given a soft(7),
// which is exactly shoe.prob( 5) * P(18, 12) + shoe.prob( 6) * P(18, 13)
//                + shoe.prob( 7) * P(18, 14) + shoe.prob( 8) * P(18, 15)
//                + shoe.prob( 9) * P(18, 16) + shoe.prob(10) * P(18, 17)
//                + shoe.prob( 1) * P(18,S08) + shoe.prob( 2) * P(18,S09)
//                + shoe.prob( 3) * P(18,S10) + shoe.prob( 4) * P(18,S11).
//
// ...
//
// P(21, S07) is the probability of getting a hard(21) given a soft(7),
// which is exactly shoe.prob( 5) * P(21, 12) + shoe.prob( 6) * P(21, 13)
//                + shoe.prob( 7) * P(21, 14) + shoe.prob( 8) * P(21, 15)
//                + shoe.prob( 9) * P(21, 16) + shoe.prob(10) * P(21, 17)
//                + shoe.prob( 1) * P(21,S08) + shoe.prob( 2) * P(21,S09)
//                + shoe.prob( 3) * P(21,S10) + shoe.prob( 4) * P(21,S11).
//
//                         ---------
//
// P(17, S06) is the probability of getting a hard(17) given a soft(6),
// which is exactly shoe.prob( 6) * P(17, 12) + shoe.prob( 7) * P(17, 13)
//                + shoe.prob( 8) * P(17, 14) + shoe.prob( 9) * P(17, 15)
//                + shoe.prob(10) * P(17, 16) + shoe.prob( 1) * P(17,S07)
//                + shoe.prob( 2) * P(17,S08) + shoe.prob( 3) * P(17,S09)
//                + shoe.prob( 4) * P(17,S10) + shoe.prob( 5) * P(17,S11).
//
// P(18, S06) is the probability of getting a hard(18) given a soft(6),
// which is exactly shoe.prob( 6) * P(18, 12) + shoe.prob( 7) * P(18, 13)
//                + shoe.prob( 8) * P(18, 14) + shoe.prob( 9) * P(18, 15)
//                + shoe.prob(10) * P(18, 16) + shoe.prob( 1) * P(18,S07)
//                + shoe.prob( 2) * P(18,S08) + shoe.prob( 3) * P(18,S09)
//                + shoe.prob( 4) * P(18,S10) + shoe.prob( 5) * P(18,S11).
//
// ...
//
// P(21, S06) is the probability of getting a hard(21) given a soft(6),
// which is exactly shoe.prob( 6) * P(21, 12) + shoe.prob( 7) * P(21, 13)
//                + shoe.prob( 8) * P(21, 14) + shoe.prob( 9) * P(21, 15)
//                + shoe.prob(10) * P(21, 16) + shoe.prob( 1) * P(21,S07)
//                + shoe.prob( 2) * P(21,S08) + shoe.prob( 3) * P(21,S09)
//                + shoe.prob( 4) * P(21,S10) + shoe.prob( 5) * P(21,S11).
//
// P(17, S05) is the probability of getting a hard(17) given a soft(5),
// which is exactly shoe.prob( 7) * P(17, 12) + shoe.prob( 8) * P(17, 13)
//                + shoe.prob( 9) * P(17, 14) + shoe.prob(10) * P(17, 15)
//                + shoe.prob( 1) * P(17,S06) + shoe.prob( 2) * P(17,S07)
//                + shoe.prob( 3) * P(17,S08) + shoe.prob( 4) * P(17,S09)
//                + shoe.prob( 5) * P(17,S10) + shoe.prob( 6) * P(17,S11).
//
// P(18, S05) is the probability of getting a hard(18) given a soft(5),
// which is exactly shoe.prob( 7) * P(18, 12) + shoe.prob( 8) * P(18, 13)
//                + shoe.prob( 9) * P(18, 14) + shoe.prob(10) * P(18, 15)
//                + shoe.prob( 1) * P(18,S06) + shoe.prob( 2) * P(18,S07)
//                + shoe.prob( 3) * P(18,S08) + shoe.prob( 4) * P(18,S09)
//                + shoe.prob( 5) * P(18,S10) + shoe.prob( 6) * P(18,S11).
//
// ...
//
// P(21, S05) is the probability of getting a hard(21) given a soft(5),
// which is exactly shoe.prob( 7) * P(21, 12) + shoe.prob( 8) * P(21, 13)
//                + shoe.prob( 9) * P(21, 14) + shoe.prob(10) * P(21, 15)
//                + shoe.prob( 1) * P(21,S06) + shoe.prob( 2) * P(21,S07)
//                + shoe.prob( 3) * P(21,S08) + shoe.prob( 4) * P(21,S09)
//                + shoe.prob( 5) * P(21,S10) + shoe.prob( 6) * P(21,S11).
//
// ...
//
// P(17, S02) is the probability of getting a hard(17) given a soft(2),
// which is exactly shoe.prob(10) * P(17, 12) + shoe.prob( 1) * P(17,S03)
//                + shoe.prob( 2) * P(17,S04) + shoe.prob( 3) * P(17,S05)
//                + shoe.prob( 4) * P(17,S06) + shoe.prob( 5) * P(17,S07)
//                + shoe.prob( 6) * P(17,S08) + shoe.prob( 7) * P(17,S09)
//                + shoe.prob( 8) * P(17,S10) + shoe.prob( 9) * P(17,S11).
//
// P(18, S02) is the probability of getting a hard(18) given a soft(2),
// which is exactly shoe.prob(10) * P(18, 12) + shoe.prob( 1) * P(18,S03)
//                + shoe.prob( 2) * P(18,S04) + shoe.prob( 3) * P(18,S05)
//                + shoe.prob( 4) * P(18,S06) + shoe.prob( 5) * P(18,S07)
//                + shoe.prob( 6) * P(18,S08) + shoe.prob( 7) * P(18,S09)
//                + shoe.prob( 8) * P(18,S10) + shoe.prob( 9) * P(18,S11).
//
// ...
//
// P(21, S02) is the probability of getting a hard(21) given a soft(2),
// which is exactly shoe.prob(10) * P(21, 12) + shoe.prob( 1) * P(21,S03)
//                + shoe.prob( 2) * P(21,S04) + shoe.prob( 3) * P(21,S05)
//                + shoe.prob( 4) * P(21,S06) + shoe.prob( 5) * P(21,S07)
//                + shoe.prob( 6) * P(21,S08) + shoe.prob( 7) * P(21,S09)
//                + shoe.prob( 8) * P(21,S10) + shoe.prob( 9) * P(21,S11).
//
// P(17, S01) is the probability of getting a hard(17) given a soft(1),
// which is exactly shoe.prob( 1) * P(17,S02) + shoe.prob( 2) * P(17,S03)
//                + shoe.prob( 3) * P(17,S04) + shoe.prob( 4) * P(17,S05)
//                + shoe.prob( 5) * P(17,S06) + shoe.prob( 6) * P(17,S07)
//                + shoe.prob( 7) * P(17,S08) + shoe.prob( 8) * P(17,S09)
//                + shoe.prob( 9) * P(17,S10) + shoe.prob(10) * P(17,S11).
//
// P(18, S01) is the probability of getting a hard(18) given a soft(1),
// which is exactly shoe.prob( 1) * P(18, 12) + shoe.prob( 2) * P(18,S03)
//                + shoe.prob( 3) * P(18,S04) + shoe.prob( 4) * P(18,S05)
//                + shoe.prob( 5) * P(18,S06) + shoe.prob( 6) * P(18,S07)
//                + shoe.prob( 7) * P(18,S08) + shoe.prob( 8) * P(18,S09)
//                + shoe.prob( 9) * P(18,S10) + shoe.prob(10) * P(18,S11).
//
// ...
//
// P(21, S01) is the probability of getting a hard(21) given a soft(1),
// which is exactly shoe.prob( 1) * P(21,S02) + shoe.prob( 2) * P(21,S03)
//                + shoe.prob( 3) * P(21,S04) + shoe.prob( 4) * P(21,S05)
//                + shoe.prob( 5) * P(21,S06) + shoe.prob( 6) * P(21,S07)
//                + shoe.prob( 7) * P(21,S08) + shoe.prob( 8) * P(21,S09)
//                + shoe.prob( 9) * P(21,S10) + shoe.prob(10) * P(21,S11).
//
// P(BJ, S01) is the probability of getting blackjack given a soft(1),
// which is exactly shoe.prob(10) * P(BJ, BJ);
{
    // rows S11-S08 -- dealer cannot hit on any of these soft counts

    for (int j = 11; j >= 8; --j) { // 'soft(j)' is the row we are filling.
        table->clearRow(bjgb::State::soft(j)); // Set all entries in row to 0
                                               // (including 'e_CBJ').

        table->prob(bjgb::State::soft(j), bjgb::DealerCount::fini(j + 10)) = 1.0;
    }

    // row S07 -- depends on rules: Whether dealer must stand on soft 17.

    table->clearRow(bjgb::State::soft(7)); // Set all entries in row to 0
                                           // (including 'e_CBJ').

    if (dealerStandsOnSoft17Flag) {
        table->prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17)) = 1.0;
    } else {
        // P(17, S07) is the probability of getting a hard(17) given a soft(7),
        // which is exactly:
        //               shoe.prob( 5) * P(17, 12) + shoe.prob( 6) * P(17, 13)
        //             + shoe.prob( 7) * P(17, 14) + shoe.prob( 8) * P(17, 15)
        //             + shoe.prob( 9) * P(17, 16) + shoe.prob(10) * P(17, 17)
        //             + shoe.prob( 1) * P(17,S08) + shoe.prob( 2) * P(17,S09)
        //             + shoe.prob( 3) * P(17,S10) + shoe.prob( 4) * P(17,S11)
        //
        // P(18, S07) is the probability of getting a hard(18) given a soft(7),
        // which is exactly:
        //               shoe.prob( 5) * P(18, 12) + shoe.prob( 6) * P(18, 13)
        //             + shoe.prob( 7) * P(18, 14) + shoe.prob( 8) * P(18, 15)
        //             + shoe.prob( 9) * P(18, 16) + shoe.prob(10) * P(18, 17)
        //             + shoe.prob( 1) * P(18,S08) + shoe.prob( 2) * P(18,S09)
        //             + shoe.prob( 3) * P(18,S10) + shoe.prob( 4) * P(18,S11)
        //
        // ...
        //
        // P(21, S07) is the probability of getting a hard(21) given a soft(7),
        // which is exactly:
        //               shoe.prob( 5) * P(21, 12) + shoe.prob( 6) * P(21, 13)
        //             + shoe.prob( 7) * P(21, 14) + shoe.prob( 8) * P(21, 15)
        //             + shoe.prob( 9) * P(21, 16) + shoe.prob(10) * P(21, 17)
        //             + shoe.prob( 1) * P(21,S08) + shoe.prob( 2) * P(21,S09)
        //             + shoe.prob( 3) * P(21,S10) + shoe.prob( 4) * P(21,S11)

        for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
            for (int i = 0; i < 10; ++i) { // index of the expression to add
                int base = 4; // starting offset from 1 (ace)
                int offset = base + i; // current     "     "  "   "
                bool softFlag = offset >= 10;

                int card = 1 + offset % 10; // 5, 6, 7, 8, 9, T, A, 2, 3, 4

                int hand = 7 + card;
                // 12, 13, 14, 15, 16, 17, S08, S09, S10, S11

                int hi = softFlag ? bjgb::State::soft(hand) : bjgb::State::hard(hand); // hand index

                assert(1 <= card);
                assert(card <= 10);

                table->prob(bjgb::State::soft(7), bjgb::DealerCount::fini(k)) +=
                    shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
            }
        }
    }

    // rows S06-S02

    for (int j = 6; j >= 2; --j) { // for each soft count S06 to S02
        table->clearRow(bjgb::State::soft(j)); // Set all entries in row to 0
                                               // (including 'e_CBJ').

        for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
            for (int i = 0; i < 10; ++i) { // index of the expression to add
                int base = 11 - j; // starting offset from 1 (ace)
                int offset = base + i; // current     "     "  "   "
                bool softFlag = offset >= 10;

                int card = 1 + offset % 10; // 6, 7, 8, 9, T, A, 2, 3, 4, 5
                                            // ...
                                            // T, A, 2, 3, 4, 5, 6, 7, 8, 9

                int hand = j + card;
                // 12,  13,  14,  15,  16, S07, S08, S09, S10, S11
                // ...
                // 12, S03, S04, S05, S06, S07, S08, S09, S10, S11

                int hi = softFlag ? bjgb::State::soft(hand) : bjgb::State::hard(hand); // hand index

                assert(1 <= card);
                assert(card <= 10);

                table->prob(bjgb::State::soft(j), bjgb::DealerCount::fini(k)) +=
                    shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
            }
        }
    }

    // row S01

    table->clearRow(bjgb::State::soft(1)); // Set all entries in row to 0
                                           // (including 'e_CBJ').

    for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
        //                  v------- Note that we don't include (TEN).
        for (int i = 0; i < 9; ++i) { // index of the expression to add
            int base = 10; // starting offset from 1 (ace)
            int offset = base + i; // current     "     "  "   "
            bool softFlag = offset >= 10;

            assert(softFlag);

            int card = 1 + offset % 10; //  A,  2,  3,  4,  5,  6, 8, 9, (T)

            int hand = 1 + card;
            // S02, S03, S04, S05, S06, S07, S08, S09, S10, (S11)

            int hi = bjgb::State::soft(hand); // hand index, always soft

            assert(1 <= card);
            assert(card <= 10);

            table->prob(bjgb::State::soft(1), bjgb::DealerCount::fini(k)) +=
                shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
        }
    }

    table->prob(bjgb::State::soft(1), bjgb::DealerCount::e_CBJ) +=
        shoe.prob(10) * table->prob(bjgb::State::e_SBJ, bjgb::DealerCount::e_CBJ);
}

static void f_h2_h10_T_0(bjgc::DealerTable *table, const bjgb::Shoe& shoe, bool dealerStandsOnSoft17Flag)
// Calculate the remaining rows: hard(10) to hard(2), TEN, ZERO
//
// E.g.,
// P(17, 10) = shoe.prob(1) * P(17,S11) + shoe.prob(2) * P(17, 12)
//           + shoe.prob(3) * P(17, 13) + shoe.prob(4) * P(17, 14)
//           + shoe.prob(5) * P(17, 15) + shoe.prob(6) * P(17, 16)
//           + shoe.prob(7) * P(17, 17) + shoe.prob(8) * P(17, 18)
//           + shoe.prob(9) * P(17, 19) + shoe.prob(T) * P(17, 20)
//
// P(18, 10) = shoe.prob(1) * P(18,S11) + shoe.prob(2) * P(18, 12)
//           + shoe.prob(3) * P(18, 13) + shoe.prob(4) * P(18, 14)
//           + shoe.prob(5) * P(18, 15) + shoe.prob(6) * P(18, 16)
//           + shoe.prob(7) * P(18, 17) + shoe.prob(8) * P(18, 18)
//           + shoe.prob(9) * P(18, 19) + shoe.prob(T) * P(18, 20)
// ...
// P(21, 10) = shoe.prob(1) * P(21,S11) + shoe.prob(2) * P(21, 12)
//           + shoe.prob(3) * P(21, 13) + shoe.prob(4) * P(21, 14)
//           + shoe.prob(5) * P(21, 15) + shoe.prob(6) * P(21, 16)
//           + shoe.prob(7) * P(21, 17) + shoe.prob(8) * P(21, 18)
//           + shoe.prob(9) * P(21, 19) + shoe.prob(T) * P(21, 20)
//
// P(17,  9) = shoe.prob(1) * P(17,S10) + shoe.prob(2) * P(17, 11)
//           + shoe.prob(3) * P(17, 12) + shoe.prob(4) * P(17, 13)
//           + shoe.prob(5) * P(17, 14) + shoe.prob(6) * P(17, 15)
//           + shoe.prob(7) * P(17, 16) + shoe.prob(8) * P(17, 17)
//           + shoe.prob(9) * P(17, 18) + shoe.prob(T) * P(17, 19)
//
// P(18,  9) = shoe.prob(1) * P(18,S10) + shoe.prob(2) * P(18, 11)
//           + shoe.prob(3) * P(18, 12) + shoe.prob(4) * P(18, 13)
//           + shoe.prob(5) * P(18, 14) + shoe.prob(6) * P(18, 15)
//           + shoe.prob(7) * P(18, 16) + shoe.prob(8) * P(18, 17)
//           + shoe.prob(9) * P(18, 18) + shoe.prob(T) * P(18, 19)
// ...
// P(21,  9) = shoe.prob(1) * P(21,S10) + shoe.prob(2) * P(21, 11)
//           + shoe.prob(3) * P(21, 12) + shoe.prob(4) * P(21, 13)
//           + shoe.prob(5) * P(21, 14) + shoe.prob(6) * P(21, 15)
//           + shoe.prob(7) * P(21, 16) + shoe.prob(8) * P(21, 17)
//           + shoe.prob(9) * P(21, 18) + shoe.prob(T) * P(21, 19)
//
// :
//
// P(17,  2) = shoe.prob(1) * P(17, S03) + shoe.prob(2) * P(17,  4)
//           + shoe.prob(3) * P(17,  5) + shoe.prob(4) * P(17,  6)
//           + shoe.prob(5) * P(17,  7) + shoe.prob(6) * P(17,  8)
//           + shoe.prob(7) * P(17,  9) + shoe.prob(8) * P(17, 10)
//           + shoe.prob(9) * P(17, 11) + shoe.prob(T) * P(17, 12)
//
// P(18,  2) = shoe.prob(1) * P(18, S03) + shoe.prob(2) * P(18,  4)
//           + shoe.prob(3) * P(18,  5) + shoe.prob(4) * P(18,  6)
//           + shoe.prob(5) * P(18,  7) + shoe.prob(6) * P(18,  8)
//           + shoe.prob(7) * P(18,  9) + shoe.prob(8) * P(18, 10)
//           + shoe.prob(9) * P(18, 11) + shoe.prob(T) * P(18, 12)
// ...
// P(21,  2) = shoe.prob(1) * P(21, S03) + shoe.prob(2) * P(21,  4)
//           + shoe.prob(3) * P(21,  5) + shoe.prob(4) * P(21,  6)
//           + shoe.prob(5) * P(21,  7) + shoe.prob(6) * P(21,  8)
//           + shoe.prob(7) * P(21,  9) + shoe.prob(8) * P(21, 10)
//           + shoe.prob(9) * P(21, 11) + shoe.prob(T) * P(21, 12)
{
    for (int j = 10; j >= 2; --j) { // 'hard(j)' is the row we are filling.
        table->clearRow(bjgb::State::hard(j)); // Set all entries in row to 0
                                               // (including 'e_CBJ').

        for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
            for (int card = 1; card <= 10; ++card) { // for each card value
                bool softFlag = 1 == card; // an Ace?
                int hand = j + card;
                int hi = softFlag ? bjgb::State::soft(hand) : bjgb::State::hard(hand);
                // hand index

                table->prob(bjgb::State::hard(j), bjgb::DealerCount::fini(k)) +=
                    shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
            }
        }
    }

    // TEN

    table->clearRow(bjgb::State::e_H_T); // Set all entries in row to 0
                                         // (including 'e_CBJ').

    for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
        for (int card = 2; card <= 10; ++card) { // for each card value except
                                                 // A
            int hand = 10 + card;
            int hi = bjgb::State::hard(hand); // hand index, always hard

            table->prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(k)) +=
                shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
        }
    }

    table->prob(bjgb::State::e_H_T, bjgb::DealerCount::e_CBJ) =
        shoe.prob(1) * table->prob(bjgb::State::e_SBJ, bjgb::DealerCount::e_CBJ);

    // ZERO

    table->clearRow(bjgb::State::e_HZR); // Set all entries in row to 0
                                         // (including 'e_CBJ').

    for (int k = 17; k <= 21; ++k) { // for each kolumn 17-21
        for (int card = 1; card <= 9; ++card) { // for each card value except T
            bool softFlag = 1 == card; // an Ace?
            int hand = 0 + card;
            int hi = softFlag ? bjgb::State::soft(hand) : bjgb::State::hard(hand); // hand index

            table->prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(k)) +=
                shoe.prob(card) * table->prob(hi, bjgb::DealerCount::fini(k));
        }

        table->prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(k)) +=
            shoe.prob(10) * table->prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(k));
    }

    for (int card = 1; card <= 9; ++card) { // for each card value except T
        bool softFlag = 1 == card; // an Ace?
        int hand = 0 + card;
        int hi = softFlag ? bjgb::State::soft(hand) : bjgb::State::hard(hand); // hand index

        table->prob(bjgb::State::e_HZR, bjgb::DealerCount::e_CBJ) +=
            shoe.prob(card) * table->prob(hi, bjgb::DealerCount::e_CBJ);
    }

    table->prob(bjgb::State::e_HZR, bjgb::DealerCount::e_CBJ) +=
        shoe.prob(10) * table->prob(bjgb::State::e_H_T, bjgb::DealerCount::e_CBJ);

    // NOTE: 220118: I think we need to check this thoroughly.
    std::cout << "*** CHECK ME! ***" << std::endl;
    std::cout << "*** CHECK ME! ***" << std::endl;
    std::cout << "*** CHECK ME! ***" << std::endl;
}

static void calculateOverColumn(bjgc::DealerTable *table)
// For each row in the specified dealer 'table', add up each entry except
// the last, then subtract the total from 1.0 to get the final column
// ('e_COV').
{
    const int k_LAST = bjgb::DealerCount::k_NUM_FINAL_COUNTS - 1;

    assert(k_LAST == bjgb::DealerCount::e_COV);

    for (int st = 0; st < bjgb::State::k_NUM_STATES; ++st) { // for each state
        // 'total' is used to calculate "over" probability.  It could be done
        // independently without cumulative error, but that's not significant.

        bjgb::Types::Double total = 0.0;

        for (int k = 0; k < k_LAST; ++k) { // for each kolumn but the last
            total += table->prob(st, k);
        }

        table->prob(st, k_LAST) = 1.0 - total; // "over" is everything else.
    }
}

static void copyRowsS01H09toZ(bjgc::DealerTable *table)
// Copy the contents of the 'e_S01' and 'e_H02 .. e_H09' rows in the
// specified dealer 'table' to the corresponding 'e_S_A .. e_H_9' rows.
// Note that row 'e_H_T' is filled by 'f_0_T_2_10' (above).
{
    for (int k = 0; k < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++k) {
        // for each kolumn
        // Populate row 'e_S_A' from row 'e_S01'.

        table->prob(bjgb::State::unus(1), k) = table->prob(bjgb::State::soft(1), k);

        // Populate rows 'e_H_2 .. e_H_9' from rows 'e_H02 .. e_H09'.

        for (int i = 2; i <= 9; ++i) {
            table->prob(bjgb::State::unus(i), k) = table->prob(bjgb::State::hard(i), k);
        }
    }
}

namespace bjgc {

// ----------------------
// struct DealerTableUtil
// ----------------------

// CLASS METHODS
void DealerTableUtil::adjustForBj(DealerTable *dst, const DealerTable& src)
// NOTE: WAIT! IS THIS CORRECT? I THINK IT IS! TRIPLE CHECK :)
// The idea is that IF a blackjack happens then there is no opportunity
// for strategy.  However, it does affect the expected value of the game.
// Specifically, that BJ pushes BJ.  Note that this is important only when
// adding more money (splitting/doubling) is involved as scaling doesn't
// change the relative values otherwise.
{
    assert(dst);

    memcpy(dst, &src, sizeof src);

    // apply to each column: 17-21, BJ, OVER

    for (int j : {1, 10}) { // for each single card, 'unus(j)'
        std::cout << "adjustForBj: j = " << j << std::endl;

        const bjgb::Types::Double probabilityNotBj = 1.0 - src.prob(bjgb::State::unus(j), bjgb::DealerCount::e_CBJ);

        for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
            dst->prob(bjgb::State::unus(j), i) /= probabilityNotBj;
        }

        dst->prob(bjgb::State::unus(j), bjgb::DealerCount::e_CBJ) = 0.0;
        // Overwrite BJ to be 0!
    }
}

void DealerTableUtil::populate(DealerTable *table, const bjgb::Shoe& shoe, bool dealerStandsOnSoft17)
{
    assert(table);

    table->reset();

    // rows hard 17-21 ('e_H17 .. e_H21'), BJ, and OVer

    for (int i = 17; i <= 21; ++i) {
        table->clearRow(bjgb::State::hard(i));
        table->prob(bjgb::State::hard(i), bjgb::DealerCount::fini(i)) = 1.0;
    }

    table->clearRow(bjgb::State::e_SBJ);
    table->prob(bjgb::State::e_SBJ, bjgb::DealerCount::e_CBJ) = 1.0;

    table->clearRow(bjgb::State::e_HOV);
    table->prob(bjgb::State::e_HOV, bjgb::DealerCount::e_COV) = 1.0;

    // rows hard 11-16 ('e_H11 .. e_H16')

    f_h11_h16(table, shoe, dealerStandsOnSoft17);

    // rows soft 1-11 ('e_S01 .. e_S11')

    f_s1_s11(table, shoe, dealerStandsOnSoft17);

    // rows hard 2-10 ('e_H02 .. e_H10'), single TEN ('e_H_T'), and 'e_HZR'

    f_h2_h10_T_0(table, shoe, dealerStandsOnSoft17);

    // Calculate the final column ('e_COV') of each row as 1 minus the rest.

    calculateOverColumn(table);

    // Transfer the 'e_S01' and 'e_H02 .. e_H09' rows to 'e_S_A .. e_H_9'.

    copyRowsS01H09toZ(table);

    // Note that rows 'e_YAA .. e_YTT' are not populated in dealer tables as
    // they are relevant to players only.
}

} // namespace bjgc
