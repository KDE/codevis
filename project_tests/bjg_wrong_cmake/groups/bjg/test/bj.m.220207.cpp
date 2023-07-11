#include <bjgb_dealercount.h>
#include <bjgb_rank.h>
#include <bjgb_rules.h>
#include <bjgb_shoe.h>
#include <bjgb_shoeutil.h>
#include <bjgb_state.h>
#include <bjgb_types.h>

#include <bjgc_dealertable.h>
#include <bjgc_dealertableutil.h>
#include <bjgc_playertable.h>
#include <bjgc_playertableutil.h>

#include <cassert>
#include <iomanip>
#include <iostream>

typedef bjgb::Types::Double Double; // shorthand

// Blackjack 2021:  John Lakos
const int k_RICHNESS = +0;
//
// This is a program that calculates blackjack odds for any given hand
// combination of dealer and player based on a set of parameterized rules
// and a specific initial state of the shoe.
//
// Question 1: what's the expected value of hitting a 16 versus standing?
//
// UNADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  16: -76.94 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -57.58
// Hit:
//  16: -66.57  -47.1 -46.38 -45.63 -44.95 -43.09 -41.48 -45.84 -50.93 -57.52
// ADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  16:  -66.7 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -54.04
// Hit:
//  16: -51.71  -47.1 -46.38 -45.63 -44.95 -43.09 -41.48 -45.84 -50.93 -53.98
//
// UNADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  16: -72.22 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -57.58
// Hit:
//  16:  -68.3 -47.33 -46.58 -45.82 -45.04 -44.29 -41.48 -45.84 -50.93 -57.52
// ADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  16: -59.87 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -54.04
// Hit:
//  16: -54.21 -47.33 -46.58 -45.82 -45.04 -44.29 -41.48 -45.84 -50.93 -53.98
//
// Look at only the top row for Surrender and only the bottom two rows for hit
// v. split, double, etc. Note that either pair of rows could be used to
// evaluate 'hit' versus 'stand' as no additional money/odds are involved, and
// the comparative values are either scaled or not by the same !BJ prob.
//
// Question 2: what's the expected value of hitting a 12 versus standing?
//
// UNADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  12: -76.94 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -57.58
// Hit:
//  12: -59.48 -25.34 -23.37 -21.35 -19.33 -17.05 -25.34 -30.78 -36.88 -44.47
// ADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  12:  -66.7 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -54.04
// Hit:
//  12: -41.47 -25.34 -23.37 -21.35 -19.33 -17.05 -25.34 -30.78 -36.88 -39.84
//
// UNADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  12: -72.22 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -57.58
// Hit:
//  12: -59.75 -25.38  -23.4 -21.38 -19.34 -17.24 -25.34 -30.78 -36.88 -44.47
// ADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  12: -59.87 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -54.04
// Hit:
//  12: -41.86 -25.38  -23.4 -21.38 -19.34 -17.24 -25.34 -30.78 -36.88 -39.84
//
// Question 3: what's the expected value of hitting a 13 versus standing?
//
// UNADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  13: -76.94 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -57.58
// Hit:
//  13: -61.25 -30.78 -29.12 -27.42 -25.73 -23.56 -29.37 -34.55 -40.39 -47.73
// ADJUSTED (dealer must stand on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  13:  -66.7 -29.28 -25.23 -21.11 -16.72 -15.37 -47.54 -51.05 -54.31 -54.04
// Hit:
//  13: -44.03 -30.78 -29.12 -27.42 -25.73 -23.56 -29.37 -34.55 -40.39 -43.38
//
// UNADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  13: -72.22 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -57.58
// Hit:
//  13: -61.89 -30.86  -29.2 -27.49 -25.77    -24 -29.37 -34.55 -40.39 -47.73
// ADJUSTED (dealer must hit on soft 17):
// Stand:  A      2      3      4      5      6      7      8      9      T
//  13: -59.87 -28.65 -24.66 -20.58 -16.47 -12.11 -47.54 -51.05 -54.31 -54.04
// Hit:
//  13: -44.95 -30.86  -29.2 -27.49 -25.77    -24 -29.37 -34.55 -40.39 -43.38
//
// -----------------------------------------------------------
// DEALER ODDS: https://www.blackjackonline.com/strategy/odds/  1 deck?
// -----------------------------------------------------------
// Dealer's odds of winding up with various hands.  (Dealer Stands on S17)
//
// Internet       Calculation       Simulation
// OV - 28.36%       0.281593       (5+ digits)
// BJ –  4.82%       0.0473373           "
// 21 –  7.36%       0.0727307           "
// 20 – 17.58%       0.180252            "
// 19 – 13.48%       0.133464            "
// 18 – 13.81%       0.139497            "
// 17 – 14.58%       0.145126            "
//
// S ZR:  0.145126  0.139497  0.133464  0.180252 0.0727307 0.0473373  0.281593
// H ZR:  0.13326   0.141507  0.135474  0.182262 0.0747406 0.0473373  0.285419
// ----------------------------------------------------------------
// Dealers Odds: hands = 100000000000, HIT_S17 = 0, 1, rseed = 10 - 39
// 0.133261%  0.141507%  0.135475%  0.182261% 0.0747397% 0.0473367%   0.28542%
//    17         18         19         20         21         BJ        OVER
//__________ __________ __________ __________ __________ __________ __________
// 0.145124%  0.1395  %  0.133462%  0.180253% 0.0727316% 0.0473368%  0.281592%
// 0.145127%  0.139497%  0.133465%  0.180252% 0.0727302% 0.047337 %  0.281592%
// 0.145126%  0.139497%  0.133465%  0.180252% 0.0727297% 0.047338 %  0.281592%
// 0.145125%  0.139499%  0.133464%  0.180252% 0.0727293% 0.047337 %  0.281593%
// 0.145125%  0.139498%  0.133467%  0.180254% 0.072729 % 0.0473367%  0.281592%
// 0.145127%  0.139498%  0.133463%  0.180253% 0.0727303% 0.0473383%  0.28159 %
// 0.145126%  0.139496%  0.133466%  0.180252% 0.0727303% 0.0473379%  0.281592%
// 0.145127%  0.139498%  0.133463%  0.180254% 0.0727302% 0.0473372%  0.28159 %
// 0.145124%  0.139496%  0.133465%  0.180253% 0.0727302% 0.0473373%  0.281594%
// 0.145125%  0.139496%  0.133463%  0.180254% 0.0727298% 0.0473358%  0.281596%
//
// 0.133259%  0.141507%  0.135475%  0.182263% 0.074741 % 0.0473361%  0.285419%
// 0.133263%  0.141508%  0.135474%  0.182259% 0.0747405% 0.0473364%  0.28542 %
// 0.133259%  0.141508%  0.135474%  0.18226 % 0.0747414% 0.0473379%  0.285419%
// 0.13326 %  0.141506%  0.135473%  0.182263% 0.0747399% 0.0473369%  0.285421%
// 0.13326 %  0.141507%  0.135475%  0.182262% 0.0747407% 0.0473384%  0.285417%
// 0.133262%  0.141508%  0.135472%  0.182264% 0.0747403% 0.0473369%  0.285417%
// 0.133259%  0.141508%  0.135476%  0.182262% 0.0747413% 0.0473368%  0.285417%
// 0.13326 %  0.141507%  0.135472%  0.182263% 0.0747413% 0.0473362%  0.28542 %
// 0.133262%  0.141504%  0.135475%  0.182262% 0.0747401% 0.0473384%  0.285418%
//
// ----------------------------------------------------------------
//
// Dealer vs. Player Odds
//
// Odds of the dealer busting based on their up card:
//
// Internet       Calculation       Simulation
// 2 – 35.30%        0.353608
// 3 – 37.56%        0.373875
// 4 – 40.28%        0.394468
// 5 – 42.89%        0.416404
// 6 – 42.08%        0.42315
// 7 – 25.99%        0.262312
// 8 – 23.86%        0.244741
// 9 – 23.34%        0.228425
// T – 21.43%        0.212109
// A – 11.65%        0.115286
//
// T:  0.111424  0.111424  0.111424  0.342194  0.0345013 0.0769231 0.212109
// 9:  0.119995  0.119995  0.350765  0.119995  0.0608238 0         0.228425
// 8:  0.128567  0.359336  0.128567  0.0693949 0.0693949 0         0.244741
// 7:  0.368566  0.137797  0.0786254 0.0786254 0.0740737 0         0.262312
// 6:  0.165438  0.106267  0.106267  0.101715  0.0971633 0         0.42315
// 5:  0.122251  0.122251  0.1177    0.113148  0.108246  0         0.416404
// 4:  0.13049   0.125938  0.121386  0.116485  0.111233  0         0.394468
// 3:  0.135034  0.130482  0.125581  0.120329  0.1147    0         0.373875
// 2:  0.139809  0.134907  0.129655  0.124026  0.117993  0         0.353608
// 1:  0.130789  0.130789  0.130789  0.130789  0.0538658 0.307692  0.115286

// ---------------------------------------------------------------
// Array size/mapping for all possible hands

// With zero  cards there are  1 hands:  0
//   "  one     "     "    "  10 hands:  S1, 2-9, TN
//   "  two     "     "    "  27   "     S2-S10, 4-20, BJ
//   "  three+  "     "    "  30   "     S1-S11,     4-21,     OVER
// A dealer can have any of   35   "     S1-S11, TN, 2-21, BJ, OVER
// Before any cards are drawn 36   "     S1-S11, TN, 2-21, BJ, OVER, 0
// Dealer can stand on only    7 values: c17-c21, cBJ, cOVER

// A player: a 4 or two 2s so we need AA, 22, 44, 66, 88, and TT as hands

// probability table:
//                 0       1       2       3       4       5       6
//                c17     c18     c19     c20     c21     cbj     cov
// 53 pair(10)  -->  53
// 52 pair( 9)
// 51 pair( 8)
// 50 pair( 7)
// 49 pair( 6)
// 48 pair( 5)
// 47 pair( 4)
// 46 pair( 3)
// 45 pair( 2)
// 44 pair( 1)  -->  44
// ---------
// 43 e_HOV     -->  43
// 42 hard(21)  -->  42
// 41 hard(20)
// 40 hard(19)
// 39 hard(18)
// 38 hard(17)
// 37 hard(16)
// 36 hard(15)
// 35 hard(14)
// 34 hard(13)
// 33 hard(12)
// 32 hard(11)
// 31 hard(10)
// 30 hard( 9)
// 29 hard( 8)
// 28 hard( 7)
// 27 hard( 6)
// 26 hard( 5)
// 25 hard( 4)
// 24 hard( 3)
// 23 hard( 2)  -->  23
// --------
// 22 unus(10)  -->  22  [also 'e_H_T']
// 21 unus( 9)
// 20 unus( 8)
// 19 unus( 7)
// 18 unus( 6)
// 17 unus( 5)
// 16 unus( 4)
// 15 unus( 3)
// 14 unus( 2)
// 13 unus(1)   -->  13
// 12 e_HZR     -->  12
// --------
// 11 e_SBJ     -->  11
// 10 soft(11)  -->  10
//  9 soft(10)
//  8 soft( 9)
//  7 soft( 8)
//  6 soft( 7)
//  5 soft( 6)
//  4 soft( 5)
//  3 soft( 4)
//  2 soft( 3)
//  1 soft( 2)
//  0 soft( 1)  -->   0

//===========================================================================
// For all strategy, unless a player can lose double on a blackjack, we need
// to divide the A and 10 card rows by the probability of not getting
// a blackjack: 1 / [ 1 - P(BJ) ].  In particular, when using the dealer's
// table to calculate the stand values that will be supplied to the 1-hit
// or player's strategy, the probability of getting a BJ has to be zeroed
// for a single TEN or ACE, and all other probabilities in that row
// (including OV) scaled accordingly.
//
// TBD: I need to review this issue to make triply sure of what I am saying.
//===========================================================================

void test1()
{
    std::cout << "test1\n\n";
    assert(0 == bjgb::State::soft(1));
    assert(10 == bjgb::State::soft(11));
    assert(11 == bjgb::State::e_SBJ);
    assert(12 == bjgb::State::e_HZR);
    assert(22 == bjgb::State::e_H_T);
    assert(23 == bjgb::State::hard(2));
    assert(24 == bjgb::State::hard(3));
    assert(25 == bjgb::State::hard(4));

    assert(41 == bjgb::State::hard(20));
    assert(42 == bjgb::State::hard(21));
    assert(43 == bjgb::State::e_HOV);

    assert(13 == bjgb::State::unus(1));
    assert(14 == bjgb::State::unus(2));
    assert(22 == bjgb::State::unus(10));

    assert(44 == bjgb::State::pair(1));
    assert(53 == bjgb::State::pair(10));
}

void test2()
{
    std::cout << "test2\n\n";

    bjgb::Shoe d;
    assert(bjgb::ShoeUtil::isUnused(d));
    assert(0 == bjgb::ShoeUtil::tenRichness(d));
    std::cout << d << '\n';

    bjgb::Shoe shoe2(2);
    assert(bjgb::ShoeUtil::isUnused(shoe2));
    assert(0 == bjgb::ShoeUtil::tenRichness(shoe2));
    std::cout << shoe2 << '\n';

    using namespace bjgb::RankLiterals;

    shoe2.setNumCardsOfRank(1_R, 9);
    assert(!bjgb::ShoeUtil::isUnused(shoe2));
    assert(0 > bjgb::ShoeUtil::tenRichness(shoe2));
    std::cout << shoe2 << '\n';

    shoe2.setNumCardsOfRank(2_R, 7);
    assert(!bjgb::ShoeUtil::isUnused(shoe2));
    assert(0 == bjgb::ShoeUtil::tenRichness(shoe2));
    std::cout << shoe2 << '\n';

    shoe2.setNumCardsOfRank(3_R, 7);
    assert(!bjgb::ShoeUtil::isUnused(shoe2));
    assert(0 < bjgb::ShoeUtil::tenRichness(shoe2));
    std::cout << shoe2 << '\n';

    std::cout << "d.p( 5) = " << d.prob(5) << '\n';
    std::cout << "d.p( 6) = " << d.prob(6) << '\n';
    std::cout << "d.p( 7) = " << d.prob(7) << '\n';
    std::cout << "d.p( 8) = " << d.prob(8) << '\n';
    std::cout << "d.p( 9) = " << d.prob(9) << '\n';
    std::cout << "d.p(10) = " << d.prob(10) << '\n';
    std::cout << "d.p( 1) = " << d.prob(1) << '\n';
    std::cout << "d.p( 2) = " << d.prob(2) << '\n';
    std::cout << "d.p( 3) = " << d.prob(3) << '\n';
    std::cout << "d.p( 4) = " << d.prob(4) << '\n';

    bjgb::Shoe dm2;
    bjgb::ShoeUtil::setTenRichness(&dm2, -2);
    bjgb::Shoe dm1;
    bjgb::ShoeUtil::setTenRichness(&dm1, -1);
    bjgb::Shoe dp0;
    bjgb::ShoeUtil::setTenRichness(&dp0, +0);
    bjgb::Shoe dp1;
    bjgb::ShoeUtil::setTenRichness(&dp1, +1);
    bjgb::Shoe dp2;
    bjgb::ShoeUtil::setTenRichness(&dp2, +2);

    std::cout << "Richness at -2: " << bjgb::ShoeUtil::tenRichness(dm2) << '\n' << dm2 << '\n';
    std::cout << "Richness at -1: " << bjgb::ShoeUtil::tenRichness(dm1) << '\n' << dm1 << '\n';
    std::cout << "Richness at +0: " << bjgb::ShoeUtil::tenRichness(dp0) << '\n' << dp0 << '\n';
    std::cout << "Richness at +1: " << bjgb::ShoeUtil::tenRichness(dp1) << '\n' << dp1 << '\n';
    std::cout << "Richness at +2: " << bjgb::ShoeUtil::tenRichness(dp2) << '\n' << dp2 << '\n';

    assert(!bjgb::ShoeUtil::isUnused(dm2));
    assert(-2 == bjgb::ShoeUtil::tenRichness(dm2));

    assert(!bjgb::ShoeUtil::isUnused(dm1));
    assert(-1 == bjgb::ShoeUtil::tenRichness(dm1));

    assert(!bjgb::ShoeUtil::isUnused(dp0));
    assert(0 == bjgb::ShoeUtil::tenRichness(dp0));

    assert(!bjgb::ShoeUtil::isUnused(dp1));
    assert(1 == bjgb::ShoeUtil::tenRichness(dp1));

    assert(!bjgb::ShoeUtil::isUnused(dp2));
    assert(2 == bjgb::ShoeUtil::tenRichness(dp2));

    bjgb::Rules rules;
}

void test3()
{
    std::cout << "test3\n\n";

    bjgb::Rules rules;
}

int main(void)
{
    std::cout << "Hello Black Jack February 2022\n\n";
    test1();
    test2();
    test3();
    std::cout << "(testing done)\n\n";

    bjgb::Shoe d(8);

    // ================
    // shoe 10-richness
    // ================

    std::cout << "SETTING DECK RICHNESS TO " << k_RICHNESS << std::endl;

    std::cout << d << '\n';

    bjgb::ShoeUtil::setTenRichness(&d, k_RICHNESS);

    std::cout << d << '\n';

    bjgb::Rules rules;
    std::cout << rules << '\n';

    bjgc::DealerTable dtab;
    dtab.reset();

    // std::cout << dtab << '\n';
    bjgc::DealerTableUtil::populate(&dtab, d, rules.dealerStandsOnSoft17());

    std::cout << "===============================================\n";
    std::cout << "=============== UNadjusted dtab ===============\n";
    std::cout << "===============================================\n";
    std::cout << dtab << '\n';

    // check soft 17 values.

    if (rules.dealerStandsOnSoft17()) {
        std::cout << "must stand on all 17" << '\n';

        assert(1.0 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17)));
        assert(0.0 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18)));
        assert(0.0 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(19)));
        assert(0.0 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(20)));
        assert(0.0 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21)));
    } else {
        std::cout << "must hit on soft 17" << '\n';
        const Double s7c17 = d.prob(5) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17))
            + d.prob(6) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(17))
            + d.prob(7) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(17))
            + d.prob(8) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(17))
            + d.prob(9) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(17))
            + d.prob(10) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(17));

        std::cout << s7c17 << " == " << dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17)) << '\n';
        assert(s7c17 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17)));

        const Double s7c18 = d.prob(5) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(18))
            + d.prob(6) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(18))
            + d.prob(7) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(18))
            + d.prob(8) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(18))
            + d.prob(9) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(18))
            + d.prob(10) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(18))
            + d.prob(1) * dtab.prob(bjgb::State::soft(8), bjgb::DealerCount::fini(18));

        std::cout << s7c18 << " == " << dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18)) << '\n';
        assert(s7c18 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18)));

        const Double s7c19 = d.prob(5) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(19))
            + d.prob(6) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(19))
            + d.prob(7) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(19))
            + d.prob(8) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(19))
            + d.prob(9) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(19))
            + d.prob(10) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(19))
            + d.prob(2) * dtab.prob(bjgb::State::soft(9), bjgb::DealerCount::fini(19));

        std::cout << s7c19 << " == " << dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(19)) << '\n';
        assert(s7c19 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(19)));

        const Double s7c20 = d.prob(5) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(20))
            + d.prob(6) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(20))
            + d.prob(7) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(20))
            + d.prob(8) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(20))
            + d.prob(9) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(20))
            + d.prob(10) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(20))
            + d.prob(3) * dtab.prob(bjgb::State::soft(10), bjgb::DealerCount::fini(20));

        std::cout << s7c20 << " == " << dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(20)) << '\n';
        assert(s7c20 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(20)));

        const Double s7c21 = d.prob(5) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21))
            + d.prob(6) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(21))
            + d.prob(7) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(21))
            + d.prob(8) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(21))
            + d.prob(9) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(21))
            + d.prob(10) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(21))
            + d.prob(4) * dtab.prob(bjgb::State::soft(11), bjgb::DealerCount::fini(21));

        std::cout << s7c21 << " == " << dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21)) << '\n';
        assert(s7c21 == dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21)));
    }

    std::cout << "s6c17" << '\n';

    const Double s6c17 = d.prob(6) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17))
        + d.prob(7) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(17))
        + d.prob(8) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(17))
        + d.prob(9) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(17))
        + d.prob(10) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(17))
        + d.prob(1) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::soft(8), bjgb::DealerCount::fini(17));

    std::cout << s6c17 << " == " << dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(17)) << '\n';
    assert(s6c17 == dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(17)));

    const Double s6c18 = d.prob(6) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(18))
        + d.prob(7) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(18))
        + d.prob(8) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(18))
        + d.prob(9) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(18))
        + d.prob(10) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(18))
        + d.prob(1) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18))
        + d.prob(2) * dtab.prob(bjgb::State::soft(8), bjgb::DealerCount::fini(18));

    std::cout << s6c18 << " == " << dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(18)) << '\n';
    assert(s6c18 == dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(18)));

    const Double s6c21 = d.prob(6) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21))
        + d.prob(7) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(21))
        + d.prob(8) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(21))
        + d.prob(1) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::soft(11), bjgb::DealerCount::fini(21));

    std::cout << s6c21 << " == " << dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(21)) << '\n';
    assert(s6c21 == dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(21)));

    std::cout << "s2c17" << '\n';
    const Double s2c17 = d.prob(10) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17))
        + d.prob(1) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17));

    std::cout << s2c17 << " == " << dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(17)) << '\n';
    assert(s2c17 == dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(17)));

    const Double s2c18 = d.prob(10) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(18))
        + d.prob(1) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(18))
        + d.prob(2) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(18))
        + d.prob(3) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(18))
        + d.prob(4) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(18))
        + d.prob(5) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18))
        + d.prob(6) * dtab.prob(bjgb::State::soft(8), bjgb::DealerCount::fini(18));

    std::cout << s2c18 << " == " << dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(18)) << '\n';
    assert(s2c18 == dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(18)));

    const Double s2c21 = d.prob(10) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21))
        + d.prob(1) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::soft(11), bjgb::DealerCount::fini(21));

    std::cout << s2c21 << " == " << dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(21)) << '\n';
    assert(s2c21 == dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(21)));

    // starting with an Ace:

    std::cout << "s1c17" << '\n';
    const Double s1c17 = d.prob(1) * dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(17))
        + d.prob(6) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(17));

    std::cout << s1c17 << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(17)) << '\n';
    assert(s1c17 == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(17)));

    const Double s1c18 = d.prob(1) * dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(18))
        + d.prob(2) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(18))
        + d.prob(3) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(18))
        + d.prob(4) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(18))
        + d.prob(5) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(18))
        + d.prob(6) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(18))
        + d.prob(7) * dtab.prob(bjgb::State::soft(8), bjgb::DealerCount::fini(18));

    std::cout << s1c18 << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(18)) << '\n';
    assert(s1c18 == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(18)));

    const Double s1c19 = d.prob(1) * dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(19))
        + d.prob(2) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(19))
        + d.prob(3) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(19))
        + d.prob(4) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(19))
        + d.prob(5) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(19))
        + d.prob(6) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(19))
        + d.prob(8) * dtab.prob(bjgb::State::soft(9), bjgb::DealerCount::fini(19));

    std::cout << s1c19 << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(19)) << '\n';
    assert(s1c19 == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(19)));

    const Double s1c20 = d.prob(1) * dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(20))
        + d.prob(2) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(20))
        + d.prob(3) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(20))
        + d.prob(4) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(20))
        + d.prob(5) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(20))
        + d.prob(6) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(20))
        + d.prob(9) * dtab.prob(bjgb::State::soft(10), bjgb::DealerCount::fini(20));

    std::cout << s1c20 << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(20)) << '\n';
    assert(s1c20 == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(20)));

    const Double s1c21 = d.prob(1) * dtab.prob(bjgb::State::soft(2), bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::soft(4), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::soft(5), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::soft(6), bjgb::DealerCount::fini(21))
        + d.prob(6) * dtab.prob(bjgb::State::soft(7), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::e_SBJ, bjgb::DealerCount::fini(21));

    std::cout << s1c21 << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(21)) << '\n';
    assert(s1c21 == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(21)));

    const Double s1cbj = d.prob(10) * dtab.prob(bjgb::State::e_SBJ, bjgb::DealerCount::e_CBJ);

    std::cout << s1cbj << " == " << dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::e_CBJ) << '\n';
    assert(s1cbj == dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::e_CBJ));

    // ========================
    // Now deal with 10-2, T, 0
    // ========================

    std::cout << "h10c17" << '\n';
    const Double h10c17 = d.prob(1) * dtab.prob(bjgb::State::soft(11), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(17))
        + d.prob(6) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(17))
        + d.prob(7) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(17))
        + d.prob(8) * dtab.prob(bjgb::State::hard(18), bjgb::DealerCount::fini(17))
        + d.prob(9) * dtab.prob(bjgb::State::hard(19), bjgb::DealerCount::fini(17))
        + d.prob(10) * dtab.prob(bjgb::State::hard(20), bjgb::DealerCount::fini(17));

    std::cout << h10c17 << " == " << dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(17)) << '\n';
    assert(h10c17 == dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(17)));

    std::cout << "h10c21" << '\n';
    const Double h10c21 = d.prob(1) * dtab.prob(bjgb::State::soft(11), bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(21))
        + d.prob(6) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(21))
        + d.prob(7) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(21))
        + d.prob(8) * dtab.prob(bjgb::State::hard(18), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::hard(19), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::hard(20), bjgb::DealerCount::fini(21));

    std::cout << h10c21 << " == " << dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(21)) << '\n';
    assert(h10c21 == dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(21)));

    std::cout << "h2c17" << '\n';
    const Double h2c17 = d.prob(1) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::hard(4), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::hard(5), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::hard(6), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::hard(7), bjgb::DealerCount::fini(17))
        + d.prob(6) * dtab.prob(bjgb::State::hard(8), bjgb::DealerCount::fini(17))
        + d.prob(7) * dtab.prob(bjgb::State::hard(9), bjgb::DealerCount::fini(17))
        + d.prob(8) * dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(17))
        + d.prob(9) * dtab.prob(bjgb::State::hard(11), bjgb::DealerCount::fini(17))
        + d.prob(10) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17));

    std::cout << h2c17 << " == " << dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(17)) << '\n';
    assert(h2c17 == dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(17)));

    std::cout << "h2c21" << '\n';
    const Double h2c21 = d.prob(1) * dtab.prob(bjgb::State::soft(3), bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::hard(4), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::hard(5), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::hard(6), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::hard(7), bjgb::DealerCount::fini(21))
        + d.prob(6) * dtab.prob(bjgb::State::hard(8), bjgb::DealerCount::fini(21))
        + d.prob(7) * dtab.prob(bjgb::State::hard(9), bjgb::DealerCount::fini(21))
        + d.prob(8) * dtab.prob(bjgb::State::hard(10), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::hard(11), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21));

    std::cout << h2c21 << " == " << dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(21)) << '\n';
    assert(h2c21 == dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(21)));

    // TEN

    std::cout << "htnc17" << '\n';
    const Double htnc17 = d.prob(1) * dtab.prob(bjgb::State::e_SBJ, bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(17))
        + d.prob(6) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(17))
        + d.prob(7) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(17))
        + d.prob(8) * dtab.prob(bjgb::State::hard(18), bjgb::DealerCount::fini(17))
        + d.prob(9) * dtab.prob(bjgb::State::hard(19), bjgb::DealerCount::fini(17))
        + d.prob(10) * dtab.prob(bjgb::State::hard(20), bjgb::DealerCount::fini(17));

    std::cout << htnc17 << " == " << dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(17)) << '\n';
    assert(htnc17 == dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(17)));

    std::cout << "htnc21" << '\n';
    const Double htnc21 = d.prob(1) * dtab.prob(bjgb::State::e_SBJ, bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::hard(12), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::hard(13), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::hard(14), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::hard(15), bjgb::DealerCount::fini(21))
        + d.prob(6) * dtab.prob(bjgb::State::hard(16), bjgb::DealerCount::fini(21))
        + d.prob(7) * dtab.prob(bjgb::State::hard(17), bjgb::DealerCount::fini(21))
        + d.prob(8) * dtab.prob(bjgb::State::hard(18), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::hard(19), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::hard(20), bjgb::DealerCount::fini(21));

    std::cout << htnc21 << " == " << dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(21)) << '\n';
    assert(htnc21 == dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(21)));

    std::cout << "htncbj" << '\n';
    const Double htncbj = d.prob(1) * dtab.prob(bjgb::State::e_SBJ, bjgb::DealerCount::e_CBJ);

    std::cout << htncbj << " == " << dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::e_CBJ) << '\n';
    assert(htncbj == dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::e_CBJ));

    // ZERO

    std::cout << "hzrc17" << '\n';
    const Double hzrc17 = d.prob(1) * dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(17))
        + d.prob(2) * dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(17))
        + d.prob(3) * dtab.prob(bjgb::State::hard(3), bjgb::DealerCount::fini(17))
        + d.prob(4) * dtab.prob(bjgb::State::hard(4), bjgb::DealerCount::fini(17))
        + d.prob(5) * dtab.prob(bjgb::State::hard(5), bjgb::DealerCount::fini(17))
        + d.prob(6) * dtab.prob(bjgb::State::hard(6), bjgb::DealerCount::fini(17))
        + d.prob(7) * dtab.prob(bjgb::State::hard(7), bjgb::DealerCount::fini(17))
        + d.prob(8) * dtab.prob(bjgb::State::hard(8), bjgb::DealerCount::fini(17))
        + d.prob(9) * dtab.prob(bjgb::State::hard(9), bjgb::DealerCount::fini(17))
        + d.prob(10) * dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(17));

    std::cout << hzrc17 << " == " << dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(17)) << '\n';
    assert(hzrc17 == dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(17)));

    std::cout << "hzrc20" << '\n';
    const Double hzrc20 = d.prob(1) * dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(20))
        + d.prob(2) * dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(20))
        + d.prob(3) * dtab.prob(bjgb::State::hard(3), bjgb::DealerCount::fini(20))
        + d.prob(4) * dtab.prob(bjgb::State::hard(4), bjgb::DealerCount::fini(20))
        + d.prob(5) * dtab.prob(bjgb::State::hard(5), bjgb::DealerCount::fini(20))
        + d.prob(6) * dtab.prob(bjgb::State::hard(6), bjgb::DealerCount::fini(20))
        + d.prob(7) * dtab.prob(bjgb::State::hard(7), bjgb::DealerCount::fini(20))
        + d.prob(8) * dtab.prob(bjgb::State::hard(8), bjgb::DealerCount::fini(20))
        + d.prob(9) * dtab.prob(bjgb::State::hard(9), bjgb::DealerCount::fini(20))
        + d.prob(10) * dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(20));

    std::cout << hzrc20 << " == " << dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(20)) << '\n';
    assert(hzrc20 == dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(20)));

    std::cout << "hzrc21" << '\n';
    const Double hzrc21 = d.prob(1) * dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::fini(21))
        + d.prob(2) * dtab.prob(bjgb::State::hard(2), bjgb::DealerCount::fini(21))
        + d.prob(3) * dtab.prob(bjgb::State::hard(3), bjgb::DealerCount::fini(21))
        + d.prob(4) * dtab.prob(bjgb::State::hard(4), bjgb::DealerCount::fini(21))
        + d.prob(5) * dtab.prob(bjgb::State::hard(5), bjgb::DealerCount::fini(21))
        + d.prob(6) * dtab.prob(bjgb::State::hard(6), bjgb::DealerCount::fini(21))
        + d.prob(7) * dtab.prob(bjgb::State::hard(7), bjgb::DealerCount::fini(21))
        + d.prob(8) * dtab.prob(bjgb::State::hard(8), bjgb::DealerCount::fini(21))
        + d.prob(9) * dtab.prob(bjgb::State::hard(9), bjgb::DealerCount::fini(21))
        + d.prob(10) * dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::fini(21));

    std::cout << hzrc21 << " == " << dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(21)) << '\n';
    assert(hzrc21 == dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::fini(21)));

    std::cout << "hzrcbj" << '\n';
    const Double hzrcbj = d.prob(1) * dtab.prob(bjgb::State::soft(1), bjgb::DealerCount::e_CBJ)
        + d.prob(10) * dtab.prob(bjgb::State::e_H_T, bjgb::DealerCount::e_CBJ);

    std::cout << hzrcbj << " == " << dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::e_CBJ) << '\n';
    assert(hzrcbj == dtab.prob(bjgb::State::e_HZR, bjgb::DealerCount::e_CBJ));

    // MAKE SURE that: unus(1), unus(2) ... unus(9), unus(10)
    //          match: soft(1), hard(2) ... hard(9), e_H_T

    std::cout << "verify that rows are copied properly" << '\n';

    for (int k = 17; k <= 23; ++k) {
        int ci = k <= 21 ? bjgb::DealerCount::fini(k) : k <= 22 ? bjgb::DealerCount::e_CBJ : bjgb::DealerCount::e_COV;

        // std::cout << "k, ci: \t" << k << "\t" << ci << std:endl;

        for (int j = 1; j <= 10; ++j) {
            int hi = 1 == j ? bjgb::State::soft(1) : 10 == j ? bjgb::State::e_H_T : bjgb::State::hard(j);

            // std::cout << "\t\tj, hi: \t" << j << "\t" << hi << std::endl;

            assert(dtab.prob(bjgb::State::unus(j), ci) == dtab.prob(hi, ci));
        }
    }

    // -----------------------------------------------------------------
    // Let's test to see if we add up with bjgc::PlayerTableUtil::eStand
    // -----------------------------------------------------------------

    // std::cout << "============ bjgc::PlayerTableUtil::eStand =========="
    //           << std::endl;

    // std::cout << "u = 1, v = 21" << std::setprecision(20)
    //           << bjgc::PlayerTableUtil::eStand(dtab, 1, 21)
    //           << std::endl;
    const Double esv_1_21 = dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::e_COV)
        + dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(17))
        + dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(18))
        + dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(19))
        + dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::e_CBJ);
    // std::cout << "  expected = " << std::setprecision(20) << esv_1_21
    //           << std::endl;
    assert(esv_1_21 == bjgc::PlayerTableUtil::eStand(dtab, 1, 21));

    // std::cout << "u = 2, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 2, 21)
    //           << std::endl;
    const Double esv_2_21 = dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::e_COV)
        + dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(17))
        + dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(18))
        + dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(19))
        + dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::e_CBJ);

    // std::cout << "  expected = " << esv_2_21 << std::endl;
    assert(esv_2_21 == bjgc::PlayerTableUtil::eStand(dtab, 2, 21));

    std::cout << "u = 3, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 3, 21) << std::endl;
    std::cout << "u = 4, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 4, 21) << std::endl;
    std::cout << "u = 5, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 5, 21) << std::endl;
    std::cout << "u = 6, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 6, 21) << std::endl;
    std::cout << "u = 7, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 7, 21) << std::endl;
    std::cout << "u = 8, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 8, 21) << std::endl;
    std::cout << "u = 9, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 9, 21) << std::endl;

    // std::cout << "u = T, v = 21" << bjgc::PlayerTableUtil::eStand(dtab, 10, 21)
    //           << std::endl;
    const Double esv_T_21 = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::e_COV)
        + dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(17))
        + dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(18))
        + dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(19))
        + dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::e_CBJ);

    // std::cout << "  expected = " << esv_T_21 << std::endl;
    assert(esv_T_21 == bjgc::PlayerTableUtil::eStand(dtab, 10, 21));

    // std::cout << "u = 5, v = 18" << bjgc::PlayerTableUtil::eStand(dtab, 5, 18)
    //           << std::endl;
    const Double esv_5_18 = dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_COV)
        + dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(17))
        // dtab.prob(bjgb::State::unus( 5),
        //           bjgb::DealerCount::fini(18))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(19))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(21))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_CBJ);

    // std::cout << "  expected = " << esv_5_18 << std::endl;
    assert(esv_5_18 == bjgc::PlayerTableUtil::eStand(dtab, 5, 18));

    // std::cout << "u = 5, v = 17" << bjgc::PlayerTableUtil::eStand(dtab, 5, 17)
    //           << std::endl;
    const Double esv_5_17 = dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_COV)
        // dtab.prob(bjgb::State::unus( 5),
        //           bjgb::DealerCount::fini(17))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(18))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(19))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(21))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_CBJ);

    // std::cout << "  expected = " << esv_5_17 << std::endl;
    assert(esv_5_17 == bjgc::PlayerTableUtil::eStand(dtab, 5, 17));

    // std::cout << "u = 5, v = 16" << bjgc::PlayerTableUtil::eStand(dtab, 5, 16)
    //           << std::endl;
    const Double esv_5_16 = dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_COV)
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(17))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(18))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(19))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(20))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::fini(21))
        - dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_CBJ);

    // std::cout << "  expected = " << esv_5_16 << std::endl;
    assert(esv_5_16 == bjgc::PlayerTableUtil::eStand(dtab, 5, 16));
    assert(esv_5_16 == bjgc::PlayerTableUtil::eStand(dtab, 5, 15));
    assert(esv_5_16 == bjgc::PlayerTableUtil::eStand(dtab, 5, 15));
    assert(esv_5_16 == bjgc::PlayerTableUtil::eStand(dtab, 5, 5));
    assert(esv_5_16 == bjgc::PlayerTableUtil::eStand(dtab, 5, 4));

#if 0
    std::cout << "============ bjgc::PlayerTableUtil::eStand =========="
              << std::endl;

    for (int i = 1; i <= 10; ++i) {
        std::cout << '\n' << i << ": " << std::endl;

        for (int j = 14; j <= 21; ++j) {
            std::cout << '\n' << j << ": ";
            std::cout << bjgc::PlayerTableUtil::eStand(dtab, i, j) << " ";
        }

        std::cout << std::endl;
    }
#endif

    // -----------------------------------
    // Now we move on to the player stand.
    // -----------------------------------

    bjgc::PlayerTable pstab;
    pstab.reset();
    bjgc::PlayerTableUtil::populatePstab(&pstab, dtab, d, rules);

    std::cout << "======================================================\n";
    std::cout << "================== Unadjusted pstab ==================\n";
    std::cout << "======================================================\n";
    std::cout << pstab << '\n';

    // some more testing is needed, but looks right. :)

    // -----------------------------------------------------------------
    // void DealerTableUtil::adjustForBj(bjgc::DealerTable        *dst,
    //                                   const bjgc::DealerTable&  src);
    // -----------------------------------------------------------------

    std::cout << "===============================================\n";
    std::cout << "================ Adjusted dtab2 ===============\n";
    std::cout << "===============================================\n";

    bjgc::DealerTable dtab2;
    bjgc::DealerTableUtil::adjustForBj(&dtab2, dtab);
    std::cout << dtab2 << '\n';

    Double v_T_BJ = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::e_CBJ);
    Double v_T_NBJ = 1 - v_T_BJ;
    Double v_T_21 = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(21));
    Double v_T_20 = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(20));
    Double v_T_17 = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(17));
    Double v_T_OV = dtab.prob(bjgb::State::unus(10), bjgb::DealerCount::e_COV);

    assert(0.0 == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::e_CBJ));
    assert(v_T_21 / v_T_NBJ == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(21)));
    assert(v_T_20 / v_T_NBJ == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(20)));
    assert(v_T_17 / v_T_NBJ == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::fini(17)));
    assert(v_T_OV / v_T_NBJ == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::e_COV));

    assert(dtab.prob(bjgb::State::unus(9), bjgb::DealerCount::fini(18))
           == dtab2.prob(bjgb::State::unus(9), bjgb::DealerCount::fini(18)));
    assert(dtab.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(20))
           == dtab2.prob(bjgb::State::unus(2), bjgb::DealerCount::fini(20)));
    assert(dtab.prob(bjgb::State::unus(5), bjgb::DealerCount::e_COV)
           == dtab2.prob(bjgb::State::unus(5), bjgb::DealerCount::e_COV));

    Double v_A_BJ = dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::e_CBJ);
    Double v_A_NBJ = 1 - v_A_BJ;
    Double v_A_21 = dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(21));
    Double v_A_18 = dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(18));
    Double v_A_OV = dtab.prob(bjgb::State::unus(1), bjgb::DealerCount::e_COV);

    assert(0.0 == dtab2.prob(bjgb::State::unus(1), bjgb::DealerCount::e_CBJ));
    assert(v_A_21 / v_A_NBJ == dtab2.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(21)));
    assert(v_A_18 / v_A_NBJ == dtab2.prob(bjgb::State::unus(1), bjgb::DealerCount::fini(18)));
    assert(v_A_OV / v_A_NBJ == dtab2.prob(bjgb::State::unus(1), bjgb::DealerCount::e_COV));

    std::cout << "===============================================\n";
    std::cout << "================ Adjusted pstab2 ==============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable pstab2;
    pstab2.reset();
    bjgc::PlayerTableUtil::populatePstab(&pstab2, dtab2, d, rules);
    std::cout << pstab2 << '\n';

    assert(pstab2.exp(bjgb::State::hard(16), 10) > pstab.exp(bjgb::State::hard(16), 10));
    assert(pstab2.exp(bjgb::State::hard(16), 9) == pstab.exp(bjgb::State::hard(16), 9));
    assert(pstab2.exp(bjgb::State::hard(16), 2) == pstab.exp(bjgb::State::hard(16), 2));
    assert(pstab2.exp(bjgb::State::hard(16), 1) > pstab.exp(bjgb::State::hard(16), 1));

    // ----------------------------------------------
    // Let's test to see we add up with set and clear
    // ----------------------------------------------
    {
        std::cout << "\ntest set and clear"
                     "\n=================="
                     "\n";

        bjgc::PlayerTable a, b;

        a.reset();
        b.reset();
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        a.exp(bjgb::State::hard(5), 9) = .1; // change a[5], 9
        assert(!bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        b.exp(bjgb::State::hard(5), 9) = .1; // change b[5], 9
        assert(!bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        b.exp(bjgb::State::hard(4), 9) = .1; // change b[4], 9
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        b.exp(bjgb::State::hard(5), 10) = .2; // change b[5], 10
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        bjgc::PlayerTableUtil::copyPlayerRow(&a, bjgb::State::hard(4), b, bjgb::State::hard(4));
        bjgc::PlayerTableUtil::copyPlayerRow(&a, bjgb::State::hard(6), b, bjgb::State::hard(6));

        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        b.exp(bjgb::State::hard(4), 10) = .2; // change b[4], 10
        assert(!bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));

        bjgc::PlayerTableUtil::copyPlayerRow(&a, bjgb::State::hard(5), b, bjgb::State::hard(5));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), b, bjgb::State::hard(4)));
        assert(bjgc::PlayerTableUtil::isSamePlayerRow(a, bjgb::State::hard(5), a, bjgb::State::hard(5)));
    }

    // -----------------------------------------------
    // Let's test to see if bjgb::Types::isValid works
    // -----------------------------------------------

    std::cout << "\n========================="
                 "\n========================="
                 "\ntest bjgb::Types::isValid"
                 "\n========================="
                 "\n========================="
                 "\n";
    {
        bjgc::PlayerTable t;
        t.reset();
        std::cout << "entry[h(2), u(1)] = " << t.exp(bjgb::State::hard(2), 1) << std::endl;
        assert(!bjgb::Types::isValid(t.exp(bjgb::State::hard(2), 1)));
        t.exp(bjgb::State::hard(2), 1) = -2.00;
        assert(bjgb::Types::isValid(t.exp(bjgb::State::hard(2), 1)));
        t.exp(bjgb::State::hard(2), 1) = 2.00;
        assert(bjgb::Types::isValid(t.exp(bjgb::State::hard(2), 1)));
    }

    // ----------------------------------------------------------------
    // Let's test to see if we add up with bjgc::PlayerTableUtil::eHit1
    // ----------------------------------------------------------------

    std::cout << "\n======================================"
                 "\nLet's test to see we add up with eHit1"
                 "\n======================================"
                 "\n\n";

    /*
       Double bjgc::PlayerTableUtil::eHit1(const bjgc::PlayerTable pst,
                                           int uc, int hv, bool sf,
                                           const bjgb::Shoe&       d)
       // Return the expected value of a player's hand, given the player table
       // for standing on any two-card value, 'pst', the dealer's up card, 'uc',
       // the *minimum* value of the hand, 'hv', whether it's a soft count,  'sf',
       // and the current shoe.  Note that column values are deliberately summed
       // in a canonical order for numerical consistency: A, 2, ..., 9, T.
   */

    // depends on blackjack rules :)  In UK you can lose double the bet. :)
    std::cout << "dtab2.hc(z(10), cbj()) = " << dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::e_CBJ) << '\n';
    std::cout << " rules.canLoseDouble() = " << rules.playerCanLoseDouble() << '\n';

    assert((0.0 == dtab2.prob(bjgb::State::unus(10), bjgb::DealerCount::e_CBJ)) == !rules.playerCanLoseDouble());

    std::cout << "1 hit: hard 16, TEN:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(17), 10)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(18), 10) + d.prob(3) * pstab2.exp(bjgb::State::hard(19), 10)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(20), 10) + d.prob(5) * pstab2.exp(bjgb::State::hard(21), 10)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 10);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 10, 16, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 16, NINE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(17), 9)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(18), 9) + d.prob(3) * pstab2.exp(bjgb::State::hard(19), 9)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(20), 9) + d.prob(5) * pstab2.exp(bjgb::State::hard(21), 9)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 9) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 9)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 9) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 9)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 9);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 9, 16, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 16, TWO:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(17), 2)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(18), 2) + d.prob(3) * pstab2.exp(bjgb::State::hard(19), 2)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(20), 2) + d.prob(5) * pstab2.exp(bjgb::State::hard(21), 2)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 2) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 2)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 2) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 2)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 2);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 2, 16, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 16, ACE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(17), 1)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(18), 1) + d.prob(3) * pstab2.exp(bjgb::State::hard(19), 1)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(20), 1) + d.prob(5) * pstab2.exp(bjgb::State::hard(21), 1)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 1);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 1, 16, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 17, TEN:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(18), 10)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(19), 10) + d.prob(3) * pstab2.exp(bjgb::State::hard(20), 10)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(21), 10) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 10);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 10, 17, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 17, NINE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(18), 9)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(19), 9) + d.prob(3) * pstab2.exp(bjgb::State::hard(20), 9)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(21), 9) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 9)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 9) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 9)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 9) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 9)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 9);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 9, 17, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 17, TWO:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(18), 2)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(19), 2) + d.prob(3) * pstab2.exp(bjgb::State::hard(20), 2)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(21), 2) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 2)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 2) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 2)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 2) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 2)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 2);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 2, 17, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 17, ACE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(18), 1)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(19), 1) + d.prob(3) * pstab2.exp(bjgb::State::hard(20), 1)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(21), 1) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 1);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 1, 17, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 20, ACE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(21), 1)
            + d.prob(2) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(3) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(4) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 1);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 1, 20, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 21, ACE:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(2) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(3) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(4) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 1) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 1)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 1);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 1, 21, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);

        assert(expected > -1.000'000'000'000'000'000'1L);
        // assert(expected < -0.999'999'999'999'999'999'9L);
        assert(expected < -0.999'999'999'999'999'999'8L);
    }

    std::cout << "1 hit: hard 21, TEN:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(2) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(3) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(4) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(5) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(6) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(7) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(8) * pstab2.exp(bjgb::State::e_HOV, 10) + d.prob(9) * pstab2.exp(bjgb::State::e_HOV, 10)
            + d.prob(10) * pstab2.exp(bjgb::State::e_HOV, 10);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 10, 21, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);

        assert(expected > -1.000'000'000'000'000'000'1L);
        // assert(expected < -0.999'999'999'999'999'999'9L);
        assert(expected < -0.999'999'999'999'999'999'8L);
    }

    std::cout << "1 hit: hard 4, 5:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(5), 5)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(6), 5) + d.prob(3) * pstab2.exp(bjgb::State::hard(7), 5)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(8), 5) + d.prob(5) * pstab2.exp(bjgb::State::hard(9), 5)
            + d.prob(6) * pstab2.exp(bjgb::State::hard(10), 5) + d.prob(7) * pstab2.exp(bjgb::State::hard(11), 5)
            + d.prob(8) * pstab2.exp(bjgb::State::hard(12), 5) + d.prob(9) * pstab2.exp(bjgb::State::hard(13), 5)
            + d.prob(10) * pstab2.exp(bjgb::State::hard(14), 5);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 5, 4, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: soft 2, 10:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::soft(3), 10)
            + d.prob(2) * pstab2.exp(bjgb::State::soft(4), 10) + d.prob(3) * pstab2.exp(bjgb::State::soft(5), 10)
            + d.prob(4) * pstab2.exp(bjgb::State::soft(6), 10) + d.prob(5) * pstab2.exp(bjgb::State::soft(7), 10)
            + d.prob(6) * pstab2.exp(bjgb::State::soft(8), 10) + d.prob(7) * pstab2.exp(bjgb::State::soft(9), 10)
            + d.prob(8) * pstab2.exp(bjgb::State::soft(10), 10) + d.prob(9) * pstab2.exp(bjgb::State::soft(11), 10)
            + d.prob(10) * pstab2.exp(bjgb::State::hard(12), 10);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 10, 2, 1, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: soft 11, A:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::hard(12), 1)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(13), 1) + d.prob(3) * pstab2.exp(bjgb::State::hard(14), 1)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(15), 1) + d.prob(5) * pstab2.exp(bjgb::State::hard(16), 1)
            + d.prob(6) * pstab2.exp(bjgb::State::hard(17), 1) + d.prob(7) * pstab2.exp(bjgb::State::hard(18), 1)
            + d.prob(8) * pstab2.exp(bjgb::State::hard(19), 1) + d.prob(9) * pstab2.exp(bjgb::State::hard(20), 1)
            + d.prob(10) * pstab2.exp(bjgb::State::hard(21), 1);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 1, 11, 1, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: soft 7, 3:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::soft(8), 3)
            + d.prob(2) * pstab2.exp(bjgb::State::soft(9), 3) + d.prob(3) * pstab2.exp(bjgb::State::soft(10), 3)
            + d.prob(4) * pstab2.exp(bjgb::State::soft(11), 3) + d.prob(5) * pstab2.exp(bjgb::State::hard(12), 3)
            + d.prob(6) * pstab2.exp(bjgb::State::hard(13), 3) + d.prob(7) * pstab2.exp(bjgb::State::hard(14), 3)
            + d.prob(8) * pstab2.exp(bjgb::State::hard(15), 3) + d.prob(9) * pstab2.exp(bjgb::State::hard(16), 3)
            + d.prob(10) * pstab2.exp(bjgb::State::hard(17), 3);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 3, 7, 1, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    std::cout << "1 hit: hard 7, 10:" << std::flush;
    {
        const Double expected = d.prob(1) * pstab2.exp(bjgb::State::soft(8), 10)
            + d.prob(2) * pstab2.exp(bjgb::State::hard(9), 10) + d.prob(3) * pstab2.exp(bjgb::State::hard(10), 10)
            + d.prob(4) * pstab2.exp(bjgb::State::hard(11), 10) + d.prob(5) * pstab2.exp(bjgb::State::hard(12), 10)
            + d.prob(6) * pstab2.exp(bjgb::State::hard(13), 10) + d.prob(7) * pstab2.exp(bjgb::State::hard(14), 10)
            + d.prob(8) * pstab2.exp(bjgb::State::hard(15), 10) + d.prob(9) * pstab2.exp(bjgb::State::hard(16), 10)
            + d.prob(10) * pstab2.exp(bjgb::State::hard(17), 10);
        std::cout << " exp = " << expected << std::flush;
        const Double actual = bjgc::PlayerTableUtil::eHit1(pstab2, 10, 7, 0, d);
        std::cout << " act = " << actual << std::endl;
        assert(expected == actual);
    }

    // -------------------------------------------------------
    // Now let's look at player agrees to take exactly one hit
    // -------------------------------------------------------

    std::cout << "===============================================\n";
    std::cout << "====*****====== unadjusted p1tab =====*****====\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable p1tab;
    p1tab.reset();
    bjgc::PlayerTableUtil::populateP1tab(&p1tab, pstab, d, rules);
    std::cout << p1tab << '\n';

    std::cout << "===============================================\n";
    std::cout << "=============== Adjusted p1tab2 ===============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable p1tab2;
    p1tab2.reset();
    bjgc::PlayerTableUtil::populateP1tab(&p1tab2, pstab2, d, rules);
    std::cout << p1tab2 << '\n';

    // ----------------------------------------------
    // Let's start to look at one column of the table
    // ----------------------------------------------

    std::cout << "\n======================================"
                 "\nLet's test to see if we add up with "
                 "bjgc::PlayerTableUtil::eHitN"
                 "\n======================================"
                 "\n\n";
    std::cout << "HELLO #0" << std::endl;

    {
        bjgc::PlayerTable t;
        t.reset();
        int cd = 10;
        bjgc::PlayerTable& q = pstab;

        std::cout << "one or more hits: hard 21, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(2) * q.exp(bjgb::State::e_HOV, cd) + d.prob(3) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(4) * q.exp(bjgb::State::e_HOV, cd) + d.prob(5) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(6) * q.exp(bjgb::State::e_HOV, cd) + d.prob(7) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(8) * q.exp(bjgb::State::e_HOV, cd) + d.prob(9) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(10) * q.exp(bjgb::State::e_HOV, cd);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, pstab, cd, 21, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            t.exp(21, cd) = actual;
        }

        std::cout << "HELLO #1" << std::endl;

#if 0
        std::cout << "one or more hits: hard 20, " <<cd<< ": " << std::flush;
        {
            const Double expected =
                                 d.prob( 1) * t.exp(bjgb::State::hard(21), cd)
                               + d.prob( 2) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 3) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 4) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 5) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 6) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 7) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 8) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob( 9) * q.exp(bjgb::State::e_HOV,    cd)
                               + d.prob(10) * q.exp(bjgb::State::e_HOV,    cd);
            std::cout << " exp = " << expected << std::flush;
            const Double actual =
                          bjgc::PlayerTableUtil::eHitN(t, pstab, cd, 20, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            t.exp(20, cd) = actual;
        }
#endif
    }

    // -------------------------------------------
    // Now let's look at player who decided to hit
    // -------------------------------------------

    std::cout << "===============================================\n";
    std::cout << "====*****====== Unadjusted phtab =====*****====\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable phtab;
    phtab.reset();
    assert(!bjgb::Types::isValid(phtab.exp(bjgb::State::soft(11), 5)));
    bjgc::PlayerTableUtil::populatePhtab(&phtab, pstab, d, rules);
    assert(bjgb::Types::isValid(phtab.exp(bjgb::State::soft(11), 5)));
    std::cout << phtab << '\n';

    std::cout << "===============================================\n";
    std::cout << "=============== Adjusted phtab2 ===============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable phtab2;
    phtab2.reset();
    bjgc::PlayerTableUtil::populatePhtab(&phtab2, pstab2, d, rules);
    std::cout << phtab2 << '\n';

    // -------------------------------------------------------------
    // Let's test to see we add up with bjgc::PlayerTableUtil::eHitN
    // -------------------------------------------------------------

    std::cout << "\n======================================================"
                 "\nLet's test to see we add up with "
                 "bjgc::PlayerTableUtil::eHitN with just pstab"
                 "\n======================================================"
                 "\n\n";

    // We are testing each dealer card from 1 to 10.

    for (int cd = 1; cd <= 10; ++cd) {
        bjgc::PlayerTable& t = phtab2;
        bjgc::PlayerTable& q = pstab2;

        std::cout << "\n\t================="
                  << "\n\t**** cd = " << cd << "\n\t================="
                  << "\n";

        assert(!bjgb::Types::isValid(t.exp(bjgb::State::e_HOV, cd)));
        // 43 e_HOV - NA
        assert(!bjgb::Types::isValid(t.exp(bjgb::State::e_HZR, cd)));
        // 12 e_HZR - NA

        std::cout << "1+ hits -- hard 21, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(2) * q.exp(bjgb::State::e_HOV, cd) + d.prob(3) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(4) * q.exp(bjgb::State::e_HOV, cd) + d.prob(5) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(6) * q.exp(bjgb::State::e_HOV, cd) + d.prob(7) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(8) * q.exp(bjgb::State::e_HOV, cd) + d.prob(9) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(10) * q.exp(bjgb::State::e_HOV, cd);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 21, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(21), cd));
            // 42 hard(21)
        }

        std::cout << "1+ hits -- hard 20, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * q.exp(bjgb::State::hard(21), cd)
                + d.prob(2) * q.exp(bjgb::State::e_HOV, cd) + d.prob(3) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(4) * q.exp(bjgb::State::e_HOV, cd) + d.prob(5) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(6) * q.exp(bjgb::State::e_HOV, cd) + d.prob(7) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(8) * q.exp(bjgb::State::e_HOV, cd) + d.prob(9) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(10) * q.exp(bjgb::State::e_HOV, cd);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 20, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(20), cd));
            // 41 hard(20)
            assert(expected == t.exp(bjgb::State::pair(10), cd));
            // 53 pair(10)
        }

        std::cout << "1+ hits -- hard 19, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * q.exp(bjgb::State::hard(20), cd)
                + d.prob(2) * q.exp(bjgb::State::hard(21), cd) + d.prob(3) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(4) * q.exp(bjgb::State::e_HOV, cd) + d.prob(5) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(6) * q.exp(bjgb::State::e_HOV, cd) + d.prob(7) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(8) * q.exp(bjgb::State::e_HOV, cd) + d.prob(9) * q.exp(bjgb::State::e_HOV, cd)
                + d.prob(10) * q.exp(bjgb::State::e_HOV, cd);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 19, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(19), cd));
            // 40 hard(19)
        }
        // ------------------------------------------------------------------

        std::cout << "\n=========================================================="
                     "\nLet's test to see we add up with "
                     "bjgc::PlayerTableUtil::eHitN with tstab an pstab"
                     "\n=========================================================="
                     "\n\n";

        std::cout << "1+h-h21, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 21, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(21), cd));
            // duplicate 42 hard(21)
        }
        // std::cout << std::setprecision(20);

        std::cout << "1+h-h20, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 20, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(20), cd));
            // duplicate 41 hard(20)
        }

        std::cout << "1+h-h19, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 19, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(19), cd));
            // duplicate 40 hard(19)
        }

        std::cout << "1+h-h18, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 18, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(18), cd));
            // 39 hard(18)
            assert(expected == t.exp(bjgb::State::pair(9), cd));
            // 52 pair(9)
        }

        std::cout << "1+h-h17, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 17, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(17), cd));
            // 38 hard(17)
        }

        std::cout << "1+h-h16, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 16, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(16), cd));
            // 37 hard(16)
            assert(expected == t.exp(bjgb::State::pair(8), cd));
            // 51 pair(8)
        }

        std::cout << "1+h-h15, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 15, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(15), cd));
            // 36 hard(15)
        }

        std::cout << "1+h-h14, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 14, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(14), cd));
            // 35 hard(14)
            assert(expected == t.exp(bjgb::State::pair(7), cd));
            // 50 pair(7)
        }

        std::cout << "1+h-h13, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 13, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(13), cd));
            // 34 hard(13)
        }

        std::cout << "1+h-h12, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::e_HOV, cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 12, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(12), cd));
            // 33 hard(12)
            assert(expected == t.exp(bjgb::State::pair(6), cd));
            // 49 pair(6)
        }

        std::cout << "1+h-h11, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 11, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(11), cd));
            // 32 hard(11)
        }

        std::cout << "============================================================\n";

        std::cout << "1+h-sbj, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 11, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
        }

        std::cout << "1+h-s11, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(21), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 11, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(11), cd));
            // 10 soft(11)
            assert(expected == t.exp(bjgb::State::e_SBJ, cd)); // 11 e_SBJ
        }

        std::cout << "1+h-s10, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 10, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(10), cd)); // 9 soft(10)
        }

        std::cout << "1+h-s9, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 9, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(9), cd)); // 8 soft(9)
        }

        std::cout << "1+h-s8, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 8, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(8), cd)); // 7 soft(8)
        }

        std::cout << "1+h-s7, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 7, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(7), cd)); // 6 soft(7)
        }

        std::cout << "1+h-s6, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 6, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(6), cd)); // 5 soft(6)
        }

        std::cout << "1+h-s5, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 5, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(5), cd)); // 4 soft(5)
        }

        std::cout << "1+h-s4, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(5), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 4, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(4), cd)); // 3 soft(4)
        }

        std::cout << "1+h-s3, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(4), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(5), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 3, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(3), cd)); // 2 soft(3)
        }

        std::cout << "1+h-s2, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(3), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(4), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(5), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 2, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(2), cd)); //  1 soft(2)
            assert(expected == t.exp(bjgb::State::pair(1), cd)); // 44 pair(1)
        }

        std::cout << "1+h-s1, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(2), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(3), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(4), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(5), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 1, true, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::soft(1), cd)); //  0 soft(1)
            assert(expected == t.exp(bjgb::State::unus(1), cd)); // 13 unus(1)
        }

        std::cout << "============================================================\n";

        std::cout << "1+h-h10, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(11), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(20), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 10, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(10), cd));
            // 31 hard(10)
            assert(expected == t.exp(bjgb::State::e_H_T, cd));
            // 22 unus(10)
            assert(expected == t.exp(bjgb::State::pair(5), cd));
            // 48 pair(5)
        }

        std::cout << "1+h-h9, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(10), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(19), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 9, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(9), cd)); // 30 hard(9)
            assert(expected == t.exp(bjgb::State::unus(9), cd)); // 21 unus(9)
        }

        std::cout << "1+h-h8, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(9), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(18), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 8, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(8), cd)); // 29 hard(8)
            assert(expected == t.exp(bjgb::State::unus(8), cd)); // 20 unus(8)
            assert(expected == t.exp(bjgb::State::pair(4), cd)); // 47 pair(4)
        }

        std::cout << "1+h-h7, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(8), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(17), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 7, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(7), cd)); // 28 hard(7)
            assert(expected == t.exp(bjgb::State::unus(7), cd)); // 19 unus(7)
        }

        std::cout << "1+h-h6, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(7), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(8), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(16), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 6, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(6), cd)); // 27 hard(6)
            assert(expected == t.exp(bjgb::State::unus(6), cd)); // 18 unus(6)
            assert(expected == t.exp(bjgb::State::pair(3), cd)); // 46 pair(3)
        }

        std::cout << "1+h-h5, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(6), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(7), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(8), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(15), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 5, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(5), cd)); // 26 hard(5)
            assert(expected == t.exp(bjgb::State::unus(5), cd)); // 17 unus(5)
        }

        std::cout << "1+h-h4, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(5), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(6), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(7), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(8), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(14), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 4, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(4), cd)); // 25 hard(4)
            assert(expected == t.exp(bjgb::State::unus(4), cd)); // 16 unus(4)
            assert(expected == t.exp(bjgb::State::pair(2), cd)); // 45 pair(2)
        }

        std::cout << "1+h-h3, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(4), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(5), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(6), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(7), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(8), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(13), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 3, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(3), cd)); // 24 hard(3)
            assert(expected == t.exp(bjgb::State::unus(3), cd)); // 15 unus(3)
        }

        std::cout << "1+h-h2, " << cd << ": " << std::flush;
        {
            const Double expected = d.prob(1) * bjgc::PlayerTableUtil::mx(bjgb::State::soft(3), cd, t, q)
                + d.prob(2) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(4), cd, t, q)
                + d.prob(3) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(5), cd, t, q)
                + d.prob(4) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(6), cd, t, q)
                + d.prob(5) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(7), cd, t, q)
                + d.prob(6) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(8), cd, t, q)
                + d.prob(7) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(9), cd, t, q)
                + d.prob(8) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(10), cd, t, q)
                + d.prob(9) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(11), cd, t, q)
                + d.prob(10) * bjgc::PlayerTableUtil::mx(bjgb::State::hard(12), cd, t, q);
            std::cout << " exp = " << expected << std::flush;
            const Double actual = bjgc::PlayerTableUtil::eHitN(t, q, cd, 2, 0, d);
            std::cout << " act = " << actual << std::endl;
            assert(expected == actual);
            assert(expected == t.exp(bjgb::State::hard(2), cd)); // 23 hard(2)
            assert(expected == t.exp(bjgb::State::unus(2), cd)); // 14 unus(2)
        }

    } // end cd loop

    std::cout << "\n\n";
    std::cout << "===============================================\n";
    std::cout << "============= ON TO DOUBLE TABLE ==============\n";
    std::cout << "===============================================\n";

    std::cout << rules << std::endl;

    // Note that we will use the adjusted tables UNLESS it is possible
    // to play the hand and then lose more than an even bet to a natural
    // blackjack.

    std::cout << "===============================================\n";
    std::cout << "=============== Unadjusted pdtab ==============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable pdtab;
    pdtab.reset();
    bjgc::PlayerTableUtil::populatePdtab(&pdtab, p1tab);
    std::cout << pdtab << '\n';

    std::cout << "===============================================\n";
    std::cout << "================ Adjusted pdtab2 ==============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable pdtab2;
    pdtab2.reset();
    bjgc::PlayerTableUtil::populatePdtab(&pdtab2, p1tab2);
    std::cout << pdtab2 << '\n';

    assert(2 * p1tab2.exp(bjgb::State::e_SBJ, 1) == pdtab2.exp(bjgb::State::e_SBJ, 1));
    assert(2 * p1tab2.exp(bjgb::State::e_SBJ, 10) == pdtab2.exp(bjgb::State::e_SBJ, 10));

    assert(2 * p1tab2.exp(bjgb::State::soft(11), 1) == pdtab2.exp(bjgb::State::soft(11), 1));
    assert(2 * p1tab2.exp(bjgb::State::soft(11), 10) == pdtab2.exp(bjgb::State::soft(11), 10));

    assert(2 * p1tab2.exp(bjgb::State::soft(2), 1) == pdtab2.exp(bjgb::State::soft(2), 1));
    assert(2 * p1tab2.exp(bjgb::State::soft(2), 10) == pdtab2.exp(bjgb::State::soft(2), 10));

    assert(2 * p1tab2.exp(bjgb::State::hard(20), 1) == pdtab2.exp(bjgb::State::hard(20), 1));
    assert(2 * p1tab2.exp(bjgb::State::hard(20), 10) == pdtab2.exp(bjgb::State::hard(20), 10));

    assert(2 * p1tab2.exp(bjgb::State::hard(4), 1) == pdtab2.exp(bjgb::State::hard(4), 1));
    assert(2 * p1tab2.exp(bjgb::State::hard(4), 10) == pdtab2.exp(bjgb::State::hard(4), 10));

    std::cout << "===============================================\n";
    std::cout << "=============== Unadjusted pxtab ==============\n";
    std::cout << "===============================================\n";

    // pxtab is pdtab augmented with split information.

    bjgc::PlayerTable pxtab;
    pxtab.reset();
    bjgc::PlayerTableUtil::populatePxtab(&pxtab, pdtab, phtab, p1tab, pstab, d, rules);
    std::cout << pxtab << '\n';

    std::cout << "===============================================\n";
    std::cout << "================ adjusted pxtab2 ==============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable pxtab2;
    pxtab2.reset();
    bjgc::PlayerTableUtil::populatePxtab(&pxtab2, pdtab2, phtab2, p1tab2, pstab2, d, rules);
    std::cout << pxtab2 << '\n';

    std::cout << "\n====================================================="
                 "\nLet's start with really simple tests on just 1 split."
                 "\n====================================================="
                 "\n\n";

    // Double bjgc::PlayerTableUtil::eSplit(const bjgc::PlayerTable& pdt,
    //                                      const bjgc::PlayerTable& pht,
    //                                      const bjgc::PlayerTable& p1t,
    //                                      const bjgc::PlayerTable& pst,
    //                                      int uc, int hv, int n,
    //                                      const bjgb::Shoe&        d,
    //                                      const bjgb::Rules&       rules)

    // for (int cd = 1; cd <= 10; ++cd)
    int cd = 6;
    {
        std::cout << "\n\t================="
                  << "\n\t**** cd = " << cd << "\n\t================="
                  << "\n";

        bjgc::PlayerTable& dt = pdtab2;
        bjgc::PlayerTable& ht = phtab2;
        bjgc::PlayerTable& ot = p1tab2;
        bjgc::PlayerTable& st = pstab2;

        Double max2val99 = -99e99;

        std::cout << "two 9's against a " << cd << ", maxHands = 2\n";
        {
            int sp = 9;
            Double exp = d.prob(1) * bjgc::PlayerTableUtil::mx3(bjgb::State::soft(sp + 1), cd, dt, ht, st)
                + d.prob(2) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 2), cd, dt, ht, st)
                + d.prob(3) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 3), cd, dt, ht, st)
                + d.prob(4) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 4), cd, dt, ht, st)
                + d.prob(5) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 5), cd, dt, ht, st)
                + d.prob(6) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 6), cd, dt, ht, st)
                + d.prob(7) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 7), cd, dt, ht, st)
                + d.prob(8) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 8), cd, dt, ht, st)
                + d.prob(9) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 9), cd, dt, ht, st)
                + d.prob(10) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 10), cd, dt, ht, st);
            exp *= 2;
            std::cout << " exp = " << exp << std::flush;
            const Double act = bjgc::PlayerTableUtil::eSplit(dt, ht, ot, st, cd, 2 * sp, 1, d, rules);
            std::cout << " act = " << act << std::endl;
            assert(exp == act);
            max2val99 = exp;
        }

        Double max3val99 = -99e99;

        std::cout << "two 9's against a " << cd << ", maxHands = 3\n";
        {
            int sp = 9;
            Double exp = d.prob(1) * bjgc::PlayerTableUtil::mx3(bjgb::State::soft(sp + 1), cd, dt, ht, st)
                + d.prob(2) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 2), cd, dt, ht, st)
                + d.prob(3) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 3), cd, dt, ht, st)
                + d.prob(4) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 4), cd, dt, ht, st)
                + d.prob(5) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 5), cd, dt, ht, st)
                + d.prob(6) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 6), cd, dt, ht, st)
                + d.prob(7) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 7), cd, dt, ht, st)
                + d.prob(8) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 8), cd, dt, ht, st)
                + d.prob(9) * max2val99
                + d.prob(10) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 10), cd, dt, ht, st);
            exp *= 2;
            std::cout << " exp = " << exp << std::flush;
            const Double act = bjgc::PlayerTableUtil::eSplit(dt, ht, ot, st, cd, 2 * sp, 2, d, rules);
            std::cout << " act = " << act << std::endl;
            assert(exp == act);
            max3val99 = exp;
        }

        Double max4val99 = -99e99;

        std::cout << "two 9's against a " << cd << ", maxHands = 4\n";
        {
            int sp = 9;
            Double exp = d.prob(1) * bjgc::PlayerTableUtil::mx3(bjgb::State::soft(sp + 1), cd, dt, ht, st)
                + d.prob(2) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 2), cd, dt, ht, st)
                + d.prob(3) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 3), cd, dt, ht, st)
                + d.prob(4) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 4), cd, dt, ht, st)
                + d.prob(5) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 5), cd, dt, ht, st)
                + d.prob(6) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 6), cd, dt, ht, st)
                + d.prob(7) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 7), cd, dt, ht, st)
                + d.prob(8) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 8), cd, dt, ht, st)
                + d.prob(9) * max3val99
                + d.prob(10) * bjgc::PlayerTableUtil::mx3(bjgb::State::hard(sp + 10), cd, dt, ht, st);
            exp *= 2;
            std::cout << " exp = " << exp << std::flush;
            const Double act = bjgc::PlayerTableUtil::eSplit(dt, ht, ot, st, cd, 2 * sp, 3, d, rules);
            std::cout << " act = " << act << std::endl;
            assert(exp == act);
            max4val99 = exp;
        }

        Double max2valAA = -99e99;

        std::cout << "two A's against a " << cd << ", maxHands = 2\n";
        {
            // ASSUMES 1 card to a split ace!

            int sp = 1;
            Double exp = d.prob(1) * st.exp(bjgb::State::soft(sp + 1), cd)
                + d.prob(2) * st.exp(bjgb::State::soft(sp + 2), cd) + d.prob(3) * st.exp(bjgb::State::soft(sp + 3), cd)
                + d.prob(4) * st.exp(bjgb::State::soft(sp + 4), cd) + d.prob(5) * st.exp(bjgb::State::soft(sp + 5), cd)
                + d.prob(6) * st.exp(bjgb::State::soft(sp + 6), cd) + d.prob(7) * st.exp(bjgb::State::soft(sp + 7), cd)
                + d.prob(8) * st.exp(bjgb::State::soft(sp + 8), cd) + d.prob(9) * st.exp(bjgb::State::soft(sp + 9), cd)
                + d.prob(10) * st.exp(bjgb::State::soft(sp + 10), cd);
            exp *= 2;
            std::cout << " exp = " << exp << std::flush;
            Double ex2 = 2 * ot.exp(bjgb::State::unus(1), cd);
            std::cout << " ex2 = " << ex2 << std::flush;

            const Double act = bjgc::PlayerTableUtil::eSplit(dt, ht, ot, st, cd, 2 * sp, 1, d, rules);
            std::cout << " act = " << act << std::endl;
            assert(exp == act);
            assert(ex2 == act);
            max2valAA = exp;
        }

        Double max3valAA = -99e99;

        std::cout << "two A's against a " << cd << ", maxHands = 3\n";
        {
            // ASSUMES 1 card to a split ace!

            int sp = 1;
            Double exp = d.prob(1) * max2valAA + d.prob(2) * st.exp(bjgb::State::soft(sp + 2), cd)
                + d.prob(3) * st.exp(bjgb::State::soft(sp + 3), cd) + d.prob(4) * st.exp(bjgb::State::soft(sp + 4), cd)
                + d.prob(5) * st.exp(bjgb::State::soft(sp + 5), cd) + d.prob(6) * st.exp(bjgb::State::soft(sp + 6), cd)
                + d.prob(7) * st.exp(bjgb::State::soft(sp + 7), cd) + d.prob(8) * st.exp(bjgb::State::soft(sp + 8), cd)
                + d.prob(9) * st.exp(bjgb::State::soft(sp + 9), cd)
                + d.prob(10) * st.exp(bjgb::State::soft(sp + 10), cd);
            exp *= 2;
            std::cout << " exp = " << exp << std::flush;

            const Double act = bjgc::PlayerTableUtil::eSplit(dt, ht, ot, st, cd, 2 * sp, 2, d, rules);
            std::cout << " act = " << act << std::endl;
            assert(exp == act);
            max3valAA = exp;
        }
    }

    // void bjgc::PlayerTableUtil::populatePlayerTable(
    //                                       bjgc::PlayerTable        *pt,
    //                                       const bjgc::PlayerTable&  pxt,
    //                                       const bjgc::PlayerTable&  pht,
    //                                       const bjgc::PlayerTable&  pst)

    // Strategy is affected by knowing Dealer doesn't have blackjack.
    // Expected value of game play is therefore affected by overall strategy.
    //  I. The dealer does not have an ACE or a TEN.  In this case the
    //     adjusted values and the regular values are the same.
    // II. Dealer has a:
    //     1  ACE
    //         d.prob(10)
    //            e_CBJ ? 0 : -1
    //         1 - d.prob(10)
    //            whatever the hand says
    //     2. TEN
    //         d.prob(1)
    //            e_CBJ ? 0 : -1
    //         1 - d.prob(1)
    //            whatever the hand says
    //
    // The player table is the probability of winning given a two card hand.
    // You can always stick, so the base line is the stick strategy.  If
    // hitting is an improvement, then you hit.  If doubling is then an
    // improvement, then you double.  Finally if the hand is splittable,
    // we do that instead of the regular entry.  But there's a catch: The
    // way we play against a TEN or an ACE has to be adjusted.  For example,
    // if the dealer has an ace, then roughly 4/13 times the player will
    // a 10 and player will tie with a blackjack and expect -1 otherwise.
    // Roughly 9/13 times the player will win according to the adjusted table.
    // So we don't need the regular table UNLESS a player can lose more
    // on a double, then we just use the that table straight.

    std::cout << "===============================================\n";
    std::cout << "=============== Unadjusted ptab ===============\n";
    std::cout << "===============================================\n";

    // pxtab is pdtab augmented with split information.

    bjgc::PlayerTable ptab;
    ptab.reset();
    bjgc::PlayerTableUtil::populatePlayerTable(&ptab, pxtab, phtab, pstab, rules);
    std::cout << ptab << '\n';
    assert(ptab.exp(bjgb::State::hard(20), 8) == pstab.exp(bjgb::State::hard(20), 8));

    std::cout << "pstab.hd(h( 4), u( 8)) = " << pstab.exp(bjgb::State::hard(4), 8) << "\n";

    std::cout << "phtab.hd(h( 4), u( 8)) = " << phtab.exp(bjgb::State::hard(4), 8) << "\n";

    std::cout << "p1tab.hd(h( 4), u( 8)) = " << p1tab.exp(bjgb::State::hard(4), 8) << "\n";

    std::cout << "pdtab.hd(h( 4), u( 8)) = " << pdtab.exp(bjgb::State::hard(4), 8) << "\n";

    std::cout << " ptab.hd(h( 4), u( 8)) = " << ptab.exp(bjgb::State::hard(4), 8) << "\n";

    assert(ptab.exp(bjgb::State::hard(4), 10) == phtab.exp(bjgb::State::hard(4), 10));

    std::cout << "pstab.hd(h(11), u( 8)) = " << pstab.exp(bjgb::State::hard(11), 8) << "\n";

    std::cout << "phtab.hd(h(11), u( 8)) = " << phtab.exp(bjgb::State::hard(11), 8) << "\n";

    std::cout << "p1tab.hd(h(11), u( 8)) = " << p1tab.exp(bjgb::State::hard(11), 8) << "\n";

    std::cout << "pdtab.hd(h(11), u( 8)) = " << pdtab.exp(bjgb::State::hard(11), 8) << "\n";

    std::cout << " ptab.hd(h(11), u( 8)) = " << ptab.exp(bjgb::State::hard(11), 8) << "\n";

    assert(ptab.exp(bjgb::State::hard(11), 8) == pxtab.exp(bjgb::State::hard(11), 8));

    assert(ptab.exp(bjgb::State::soft(10), 5) == pstab.exp(bjgb::State::soft(10), 5));
    // assert(ptab.exp(bjgb::State::soft(3), 5)
    //    == phtab.exp(bjgb::State::soft(3), 5));  // EVEN
    assert(ptab.exp(bjgb::State::soft(8), 5) == pxtab.exp(bjgb::State::soft(8), 5));

    assert(ptab.exp(bjgb::State::e_SBJ, 10) == pstab.exp(bjgb::State::e_SBJ, 10));
    assert(ptab.exp(bjgb::State::e_SBJ, 6) == pstab.exp(bjgb::State::e_SBJ, 6));
    assert(ptab.exp(bjgb::State::e_SBJ, 1) == pstab.exp(bjgb::State::e_SBJ, 1));

    assert(ptab.exp(bjgb::State::pair(1), 10) == pxtab.exp(bjgb::State::pair(1), 10));
    assert(ptab.exp(bjgb::State::pair(5), 9) == pxtab.exp(bjgb::State::pair(5), 9));
    assert(ptab.exp(bjgb::State::pair(10), 1) == pxtab.exp(bjgb::State::pair(10), 1));

    std::cout << "===============================================\n";
    std::cout << "================ adjusted ptab2 ===============\n";
    std::cout << "===============================================\n";

    bjgc::PlayerTable ptab2;
    ptab2.reset();
    bjgc::PlayerTableUtil::populatePlayerTable(&ptab2, pxtab2, phtab2, pstab2, rules);
    std::cout << ptab2 << '\n';

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // THESE TESTS ARE DESIGNED SPECIFICALLY TO CHECK CLOSE HANDS:
    // ___________________________________________________________

    std::cout << "\nrichness = " << bjgb::ShoeUtil::tenRichness(d) << std::endl;

    if (0 != bjgb::ShoeUtil::tenRichness(d)) {
        std::cout << "\t=====================\n"
                     "\tSKIPPING CLOSE COUNTS\n"
                     "\t=====================\n";
    } else {
        // HARD COUNTS

        // double
        assert(ptab2.exp(bjgb::State::hard(11), 10) == pdtab2.exp(bjgb::State::hard(11), 10));

        std::cout << "pstab2.hd(h(11), u( 1)) = " << pstab2.exp(bjgb::State::hard(11), 1) << "\n";

        std::cout << "phtab2.hd(h(11), u( 1)) = " << phtab2.exp(bjgb::State::hard(11), 1) << "\n";

        std::cout << "p1tab2.hd(h(11), u( 1)) = " << p1tab2.exp(bjgb::State::hard(11), 1) << "\n";

        std::cout << "pdtab2.hd(h(11), u( 1)) = " << pdtab2.exp(bjgb::State::hard(11), 1) << "\n";

        std::cout << " ptab2.hd(h(11), u( 1)) = " << ptab2.exp(bjgb::State::hard(11), 1) << "\n";

        if (rules.dealerStandsOnSoft17()) {
            assert(ptab2.exp(bjgb::State::hard(11), 1) == phtab2.exp(bjgb::State::hard(11), 1)); // EVEN
        } else {
            assert(ptab2.exp(bjgb::State::hard(11), 1) == pdtab2.exp(bjgb::State::hard(11), 1));
        }

        assert(ptab2.exp(bjgb::State::hard(10), 2) == pdtab2.exp(bjgb::State::hard(10), 2));
        assert(ptab2.exp(bjgb::State::hard(10), 9) == pdtab2.exp(bjgb::State::hard(10), 9));
        assert(ptab2.exp(bjgb::State::hard(9), 6) == pdtab2.exp(bjgb::State::hard(9), 6));

        std::cout << "pstab2.hd(h( 9), u( 4)) = " << pstab2.exp(bjgb::State::hard(9), 4) << "\n";

        std::cout << "phtab2.hd(h( 9), u( 4)) = " << phtab2.exp(bjgb::State::hard(9), 4) << "\n";

        std::cout << "p1tab2.hd(h( 9), u( 4)) = " << p1tab2.exp(bjgb::State::hard(9), 4) << "\n";

        std::cout << "pdtab2.hd(h( 9), u( 4)) = " << pdtab2.exp(bjgb::State::hard(9), 4) << "\n";

        std::cout << " ptab2.hd(h( 9), u( 4)) = " << ptab2.exp(bjgb::State::hard(9), 4) << "\n";

        assert(ptab2.exp(bjgb::State::hard(9), 4) == pdtab2.exp(bjgb::State::hard(9), 4));

        std::cout << "pstab2.hd(h( 9), u( 3)) = " << pstab2.exp(bjgb::State::hard(9), 3) << "\n";

        std::cout << "phtab2.hd(h( 9), u( 3)) = " << phtab2.exp(bjgb::State::hard(9), 3) << "\n";

        std::cout << "p1tab2.hd(h( 9), u( 3)) = " << p1tab2.exp(bjgb::State::hard(9), 3) << "\n";

        std::cout << "pdtab2.hd(h( 9), u( 3)) = " << pdtab2.exp(bjgb::State::hard(9), 3) << "\n";

        std::cout << " ptab2.hd(h( 9), u( 3)) = " << ptab2.exp(bjgb::State::hard(9), 3) << "\n";

        assert(ptab2.exp(bjgb::State::hard(9), 3) == pdtab2.exp(bjgb::State::hard(9), 3));

        // hit
        assert(ptab2.exp(bjgb::State::hard(16), 10) == phtab2.exp(bjgb::State::hard(16), 10)); // EVEN
        assert(ptab2.exp(bjgb::State::hard(10), 10) == phtab2.exp(bjgb::State::hard(10), 10));
        assert(ptab2.exp(bjgb::State::hard(9), 9) == phtab2.exp(bjgb::State::hard(9), 9));
        assert(ptab2.exp(bjgb::State::hard(12), 2) == phtab2.exp(bjgb::State::hard(12), 2)); // EVEN
        assert(ptab2.exp(bjgb::State::hard(12), 3) == phtab2.exp(bjgb::State::hard(12), 3)); // EVEN
        assert(ptab2.exp(bjgb::State::hard(9), 2) == phtab2.exp(bjgb::State::hard(9), 2));

        // stand
        assert(ptab2.exp(bjgb::State::hard(17), 10) == pstab2.exp(bjgb::State::hard(17), 10));
        assert(ptab2.exp(bjgb::State::hard(12), 4) == pstab2.exp(bjgb::State::hard(12), 4));
        assert(ptab2.exp(bjgb::State::hard(13), 2) == pstab2.exp(bjgb::State::hard(13), 2));

        // SOFT COUNTS

        // double
        assert(ptab2.exp(bjgb::State::soft(8), 6) == pdtab2.exp(bjgb::State::soft(8), 6));
        assert(ptab2.exp(bjgb::State::soft(8), 3) == pdtab2.exp(bjgb::State::soft(8), 3));
        assert(ptab2.exp(bjgb::State::soft(7), 4) == pdtab2.exp(bjgb::State::soft(7), 4));

        // hit
        assert(ptab2.exp(bjgb::State::soft(3), 4) == phtab2.exp(bjgb::State::soft(3), 4));
        assert(ptab2.exp(bjgb::State::soft(2), 5) == phtab2.exp(bjgb::State::soft(2), 5));
        assert(ptab2.exp(bjgb::State::soft(8), 10) == phtab2.exp(bjgb::State::soft(8), 10));
        assert(ptab2.exp(bjgb::State::soft(8), 9) == phtab2.exp(bjgb::State::soft(8), 9));
        assert(ptab2.exp(bjgb::State::soft(7), 1) == phtab2.exp(bjgb::State::soft(7), 1));

        // stick
        assert(ptab2.exp(bjgb::State::soft(8), 8) == pstab2.exp(bjgb::State::soft(8), 8));
        assert(ptab2.exp(bjgb::State::soft(8), 7) == pstab2.exp(bjgb::State::soft(8), 7));

        std::cout << "pstab2.hd(s( 8), u( 2)) = " << pstab2.exp(bjgb::State::soft(8), 2) << "\n";

        std::cout << "phtab2.hd(s( 8), u( 2)) = " << phtab2.exp(bjgb::State::soft(8), 2) << "\n";

        std::cout << "p1tab2.hd(s( 8), u( 2)) = " << p1tab2.exp(bjgb::State::soft(8), 2) << "\n";

        std::cout << "pdtab2.hd(s( 8), u( 2)) = " << pdtab2.exp(bjgb::State::soft(8), 2) << "\n";

        std::cout << " ptab2.hd(s( 8), u( 2)) = " << ptab2.exp(bjgb::State::soft(8), 2) << "\n";

        if (rules.dealerStandsOnSoft17()) {
            assert(ptab2.exp(bjgb::State::soft(8), 2) == pstab2.exp(bjgb::State::soft(8), 2));
        } else {
            assert(ptab2.exp(bjgb::State::soft(8), 2) == pdtab2.exp(bjgb::State::soft(8), 2));
        }

    } // end close counts

    // Call function to calculate odds of winning

    std::cout << "\t===================\n"
                 "\tEXPECTED DECK VALUE\n"
                 "\t===================\n";

    Double eVal = bjgc::PlayerTableUtil::evalShoeImp(d, ptab, ptab2, rules);

    std::cout << "\teVal = " << eVal << "\n";

    // start interpreter ?
}
