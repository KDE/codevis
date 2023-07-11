// bjgc_playertableutil.cpp                                           -*-C++-*-
#include <bjgc_playertableutil.h>

#include <bjgb_dealercount.h>
#include <bjgb_types.h> // 'Double'

#include <cassert>
#include <iostream> // TBD TEMPORARY

// The player's hand is all about expected value, not just Probability.
//
// The question is: For a specified set of rules, shoe, and dealer up card,
// what is the expected value of any given hand (assuming you play perfectly)
// and what is the optimal strategy.
//
// The structure of the Player's table is the same as that of the Dealer's
// table except that the numbers in the columns represent expected values for
// each dealer up card assuming a strategy.
//
// We need a table for stand (pstab), hit at least once (phtab), and hit once
// only (p1tab), which we will use when we double the bet.
//
// In the end, we will need a table that has the best way to play.
//
// Split applies only to certain even counts: S2, 4-20.  We will get one card
// to a split Ace unless we there is rule allowing multiple cards.  In the end,
// we will have to incorporate the probability of getting two of the same card:
// P(1) * P(1), P(2) * P(2), ... P(10) * P(10).  In the case of Aces, we will
// typically use the 1-hit table without considering splits.  In all other
// cases, we'll need a table that considers the possibility of getting a pair
// and doubling the resulting expected value of that single card.  As a result,
// the dealer table holds soft counts at the beginning and the has notion of
// single cards, even for non-TEN cards, so that we can use the hit/one-hit
// depending on whether it is a pair of Aces or otherwise.
//
// Surrender is defined precisely in terms of whether the expected value of
// -0.5 is better or worse than playing normally.

namespace bjgc {

// ----------------------
// struct PlayerTableUtil
// ----------------------

// CLASS METHODS
void PlayerTableUtil::copyPlayerRow(PlayerTable *pta, int hia, const PlayerTable& ptb, int hib)
{
    assert(pta);
    assert(0 <= hia);
    assert(hia < bjgb::State::k_NUM_STATES);
    assert(0 <= hib);
    assert(hib < bjgb::State::k_NUM_STATES);

    for (int i = 1; i <= 10; ++i) {
        pta->exp(hia, i) = ptb.exp(hib, i);
    }
}

bjgb::Types::Double PlayerTableUtil::eDouble(const PlayerTable& p1t, int uc, int hv, bool sf)
{
    std::cout << "##### eDouble: " << std::flush;
    std::cout << " uc = " << uc;
    std::cout << " hv = " << hv;
    std::cout << " sf = " << sf;
    std::cout << std::endl;

    assert(1 <= uc);
    assert(uc <= 10);
    assert(2 <= hv);
    assert(hv <= 21);
    assert(hv >= 4 || sf);
    assert(hv <= 11 || !sf);

    // The expected value is simply twice the corresponding value in the
    // hard/soft hand.

    int hi = sf ? bjgb::State::soft(hv) : bjgb::State::hard(hv); // hand index

    bjgb::Types::Double temp = p1t.exp(hi, uc);
    assert(bjgb::Types::isValid(temp));

    bjgb::Types::Double expectedValue = 2 * temp;
    assert(bjgb::Types::isValid(expectedValue));

    return expectedValue;
}

bjgb::Types::Double PlayerTableUtil::eHit1(const PlayerTable& pst, int uc, int hv, bool sf, const bjgb::Shoe& shoe)
{
    // Implementation note: Column values are deliberately summed in a
    // canonical order for numerical consistency: A, 2, ..., 9, T.

    assert(1 <= uc);
    assert(uc <= 10);
    assert(1 <= hv);
    assert(hv <= 21);
    assert(hv > 1 || sf);
    assert(hv <= 11 || !sf);

    bjgb::Types::Double sum = 0; // Accumulated expected value for each
                                 // possible hand.

    for (int cd = 1; cd <= 10; ++cd) { // for each possible card dealt, 'cd'
        int soft = 1 == cd || sf; // if we get an A it is automatically soft
        int value = hv + cd; // hit hand with 'cd'

        if (soft && value + 10 <= 21) {
            value += 10; // make soft count a hard count
        }

        // Hand 'value' is hard from here on.

        if (value <= 16) {
            assert(bjgb::Types::isValid(pst.exp(bjgb::State::hard(16), uc)));

            sum += shoe.prob(cd) * pst.exp(bjgb::State::hard(16), uc);
        } else if (value >= 22) {
            assert(-1 == pst.exp(bjgb::State::e_HOV, uc));

            sum += shoe.prob(cd) * pst.exp(bjgb::State::e_HOV, uc);
        } else {
            assert(17 <= value);
            assert(value <= 21);
            assert(bjgb::Types::isValid(pst.exp(bjgb::State::hard(value), uc)));

            sum += shoe.prob(cd) * pst.exp(bjgb::State::hard(value), uc);
        }
    }

    return sum;
}

bjgb::Types::Double
PlayerTableUtil::eHitN(const PlayerTable& pht, const PlayerTable& pst, int uc, int hv, bool sf, const bjgb::Shoe& shoe)
{
    // Implementation note: 'eHitN' is similar to 'eHit1' except that, for
    // each card, we choose to hit again or stand as appropriate.  Since this
    // table depends on itself, we need to do things in order, which means hard
    // counts down to 11 first, then soft counts from 11 to 1, followed by hard
    // counts from 10 to 2.  If we access an expected value that is not in the
    // valid range of -2 to 2, it is invalid and should be asserted as such.
    // Note that column values are deliberately summed in a canonical order for
    // numerical consistency: A, 2, ..., 9, T.

    assert(1 <= uc);
    assert(uc <= 10);
    assert(1 <= hv);
    assert(hv <= 21);
    assert(hv > 1 || sf);
    assert(hv <= 11 || !sf);

    bjgb::Types::Double sum = 0; // Accumulated expected value for each
                                 // possible hand.

    for (int cd = 1; cd <= 10; ++cd) { // for each possible card dealt, 'cd'
        int minValue = hv + cd; // lowest possible cards value

        bjgb::Types::Double expVal = -9999.99;
        bjgb::Types::Double shi = -999;
        bjgb::Types::Double hhi = -999;
        bjgb::Types::Double maxValue = -999999.9999;

        bool stillSoftFlag; // unset!

        // If minimum value is over, the expected value is exactly that of
        // standing with an OVER.

        if (minValue > 21) {
            expVal = pst.exp(bjgb::State::e_HOV, uc);
            assert(-1.0 == expVal);
        } else {
            stillSoftFlag = (1 == cd || sf) && minValue <= 11;
            maxValue = minValue + stillSoftFlag * 10;

            shi = bjgb::State::hard(maxValue);
            hhi = (stillSoftFlag ? bjgb::State::soft : bjgb::State::hard)(minValue);
            bjgb::Types::Double stand = pst.exp(shi, uc);
            assert(bjgb::Types::isValid(stand));

            bjgb::Types::Double hitIt = pht.exp(hhi, uc);
            assert(bjgb::Types::isValid(hitIt));

            expVal = std::max<bjgb::Types::Double>(stand, hitIt);
            assert(bjgb::Types::isValid(expVal));
        }

        sum += shoe.prob(cd) * expVal;
        assert(bjgb::Types::isValid(sum));

        // what follows is just double checking :)

        if (minValue > 21) {
            assert(-1.0 == expVal);
        } else if (minValue <= 11 && stillSoftFlag) {
            assert(maxValue == minValue + 10);
            assert(maxValue <= 21);
            assert(bjgb::State::hard(maxValue) == shi);
            assert(bjgb::State::soft(minValue) == hhi);
        } else { // they are both hard counts and not over 21.
            assert(maxValue == minValue);
            assert(maxValue == minValue);
            assert(maxValue <= 21);
            assert(bjgb::State::hard(maxValue) == shi);
            assert(bjgb::State::hard(minValue) == hhi);
        }
    }

    return sum;
}

bjgb::Types::Double PlayerTableUtil::eSplit(const PlayerTable& pdt,
                                            const PlayerTable& pht,
                                            const PlayerTable& p1t,
                                            const PlayerTable& pst,
                                            int uc,
                                            int hv,
                                            int n,
                                            const bjgb::Shoe& shoe,
                                            const bjgb::Rules& rules)
// TBD: rules: can you double after a split Ace?
// TBD: rules: can you re-split after a split Ace?
{
    std::cout << "[eSplit: n = " << n << "]" << std::flush;
    assert(1 <= uc);
    assert(uc <= 10);
    assert(2 <= hv);
    assert(hv <= 20);
    assert(0 == hv % 2); // only even hands can be split
    assert(n >= 0);

    // The value to be returned is 2 * the expected value of every drawn card
    // other than the split card.  Suppose the split card is 8: the expected
    // value of splitting two eights is twice the max of hitting or doubling
    // A..7, 9, and 10.  But, for an eight, it is recursive, and there is
    // P(8) * the expected value of splitting again with decremented 'n':
    //
    // Split(8s, 10, 3) = 2 * shoe.prob( 1) * maxDHS(soft( 9))  // maybe can't
    //                                                          // double?
    //                  + 2 * shoe.prob( 2) * maxDHS(hard(10))
    //                  + 2 * shoe.prob( 3) * maxDHS(hard(11))
    //                  + ...
    //                  + 2 * shoe.prob( 7) * maxDHS(hard(15))
    //                  + 2 * shoe.prob( 8) * Split(8s, 10, 2)
    //                  + 2 * shoe.prob( 9) * maxDHS(hard(17))
    //                  + 2 * shoe.prob(10) * maxDHS(hard(18))
    //
    // Split(As, 6, 1)  = 2 * shoe.prob( 1) * maxDHS(soft(2))  // maybe can't
    //                                                         // double?
    //                  + 2 * shoe.prob( 2) * maxDHS(soft(3))
    //                  + 2 * shoe.prob( 3) * maxDHS(hard(11))
    //                  + ...
    //                  + 2 * shoe.prob( 7) * maxDHS(hard(15))
    //                  + 2 * shoe.prob( 9) * maxDHS(hard(17))
    //                  + 2 * shoe.prob(10) * maxDHS(hard(18))
    //

    if (0 == n) { // Splitting is not an option.
        std::cout << "[n = 0]" << std::flush;
        // We have recursed, and splitting is no longer an option.  We need to
        // return the best value between hitting or standing, and possibly
        // doubling.

        int hi = 2 == hv ? bjgb::State::soft(2) : bjgb::State::hard(hv);
        int ui = uc; // It would be nice if both
                     // 'hi' and 'ui' were type-safe.

        // TBD: the block below should be factored out as a separate function.

        bjgb::Types::Double hVal = pht.exp(hi, ui);
        bjgb::Types::Double sVal = pst.exp(hi, ui);

        bjgb::Types::Double expVal = std::max<bjgb::Types::Double>(hVal, sVal);

        if (rules.playerMayDoubleOnTheseTwoCards(hv, 2 == hv)) {
            std::cout << "[can double]" << std::flush;
            bjgb::Types::Double dVal = pdt.exp(hi, ui);
            expVal = std::max<bjgb::Types::Double>(expVal, dVal);
        }

        std::cout << "[***EARLY RETURN***]" << std::flush;

        return expVal; // there expected value isn't doubled here
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // If we get to here, splitting is a viable option and the bet will be
    // doubled; we'll make the assumption that splitting is to be repeated
    // while possible.

    bjgb::Types::Double expectedValue = 0.0; // Accumulate expected value over
                                             // 10 cards.
    int ui = uc;
    const int playerCard = hv / 2;

    // Address splitting Aces as a special case.  Note that re-splitting Aces
    // is handled from outside the function by passing num-splits as 1.  There
    // is therefore no need to query 'playerMayResplitAces'.

    if (1 == playerCard && rules.playerGetsOneCardOnlyToSplitAce()) {
        std::cout << "[card=A]" << std::flush;
        bool resplitAces = n >= 2; // set by checking rules from outside
        std::cout << "[A]" << std::flush;

        if (resplitAces) { // Note that we do 'prob(1)' first! always.
            std::cout << "[C]" << std::flush;
            // TBD look into removing possibility of 'n == 0'
            expectedValue += shoe.prob(1) * eSplit(pdt, pht, p1t, pst, uc, hv, n - 1, shoe, rules);
        }

        for (int i = 2 - !resplitAces; i <= 10; ++i) {
            std::cout << "[B]" << std::flush;
            int softHand = 1 + i;
            expectedValue += shoe.prob(i) * pst.exp(bjgb::State::soft(softHand), ui);
        }
    } else { // treat Aces just like any other card (split based on 'n')
        std::cout << "[C]" << std::flush;
        for (int i = 1; i <= 10; ++i) { // for each possible drawn card
            std::cout << "[E]" << std::flush;
            if (playerCard == i && n >= 2) { // this is where we recurse
                std::cout << "[F]" << std::flush;
                // 0 would mean we recursed internally
                // 1 would mean no further splits are allowed
                // TBD consider removing the duplication of '0 == n'

                expectedValue += shoe.prob(playerCard) * eSplit(pdt, pht, p1t, pst, uc, hv, n - 1, shoe, rules);
            } else { // no more splitting
                std::cout << "[G]" << std::flush;
                bool sf = 1 == i || 1 == playerCard;
                int hand = playerCard + i;
                int hi = sf ? bjgb::State::soft(hand) : bjgb::State::hard(hand);

                bjgb::Types::Double hVal = pht.exp(hi, ui);
                bjgb::Types::Double sVal = pst.exp(hi, ui);

                bjgb::Types::Double expVal = std::max<bjgb::Types::Double>(hVal, sVal);

                if (rules.playerMayDoubleOnTheseTwoCards(hv, 2 == hv)) {
                    std::cout << "[H]" << std::flush;
                    bjgb::Types::Double dVal = pdt.exp(hi, ui);
                    expVal = std::max<bjgb::Types::Double>(expVal, dVal);
                }

                expectedValue += shoe.prob(i) * expVal;
                std::cout << "\n[" << i << ": " << expectedValue << "]" << std::flush;
            }
        }
    }

    std::cout << "[RETURN 2*EXP]" << std::flush;

    return 2 * expectedValue;
}

bjgb::Types::Double PlayerTableUtil::eStand(const DealerTable& t, int uc, int hv)
{
    // Implementation note: Values are deliberately added in a canonical order
    // for numerical consistency: OVER first, then lower cards, then higher
    // cards.

    assert(1 <= uc);
    assert(uc <= 10);
    assert(4 <= hv);
    assert(hv <= 21);

    const int ci = hv < 17 ? -1 : hv - 17; // count index
                                           // e.g., if hv = 17, ci = 0; if hv = 21, ci = 4; bj = 5

    bjgb::Types::Double sum = t.prob(bjgb::State::unus(uc), bjgb::DealerCount::e_COV);
    // you win if you stand and dealer busts

    for (int i = 0; i < ci; ++i) { // counts that are lower than 'hv'
        sum += t.prob(bjgb::State::unus(uc), i);
    }

    const int k_FINAL = bjgb::DealerCount::k_NUM_FINAL_COUNTS - 1;
    // Don't count OVER again.

    for (int i = ci + 1; i < k_FINAL; ++i) {
        sum -= t.prob(bjgb::State::unus(uc), i);
    }

    return sum;
}

bjgb::Types::Double PlayerTableUtil::evalShoeImp(const bjgb::Shoe& shoe,
                                                 const PlayerTable& pt,
                                                 const PlayerTable& pat,
                                                 const bjgb::Rules& rules)
{
    // The idea is that we're going to calculate the probability of every
    // dealer card.  Note that we need to pass in the adjusted player table
    // if we cannot lose double our bet, otherwise we need to pass in the
    // unadjusted table.

    // We need to make a pass for surrender if available before we check for
    // blackjack.  TBD

    bjgb::Types::Double expectedValue = 0.0;

    for (int i = 1; i <= 10; ++i) { // for each dealer card
        bjgb::Types::Double pi = shoe.prob(i);
        bjgb::Types::Double pdbj = 1 == i ? shoe.prob(10) : 10 == i ? shoe.prob(1) : 0.0;

        for (int j = 1; j <= 10; ++j) { // for each dealer card
            bjgb::Types::Double pj = shoe.prob(j);
            bjgb::Types::Double pij = pi * pj;

            for (int k = 1; k <= 10; ++k) { // for each dealer card
                bjgb::Types::Double pk = shoe.prob(k);
                bjgb::Types::Double pijk = pij * pk;

                bool softFlag = j == 1 || k == 1;
                bool splitFlag = j == k;
                int v = j + k;

                const PlayerTable& pTab = rules.playerCanLoseDouble() ? pt : pat;

                bjgb::Types::Double best = pTab.exp(softFlag ? bjgb::State::soft(v) : bjgb::State::hard(v), i);

                if (splitFlag) {
                    best = std::max<bjgb::Types::Double>(best, pTab.exp(bjgb::State::pair(j), i));
                }

                // We need to handle BJ separately because of payouts and
                // such.  We might build them directly into the expected
                // values for the tables, but let's just calculate them here
                // separately anyway:
                //
                // if player has blackjack (assume player always stands)
                //     if dealer has blackjack
                //         eVal = 0
                //     else
                //         eVal = rules.playerBlackjackPayout();
                // else
                //     if dealer has blackjack
                //         eVal = -1.0
                //     else
                //         eVal = best (based on appropriate player table)

                bool bjFlag = 11 == v && softFlag;

                bjgb::Types::Double eVal = -99e99; // "unset" value

                if (bjFlag) {
                    eVal = pdbj * 0.0 + (1.0 - pdbj) * rules.playerBlackjackPayout();
                } else {
                    eVal = pdbj * -1.0 + (1.0 - pdbj) * best;
                }

                if (rules.playerMaySurrender()) {
                    // If surrender is possible, we cannot do worse than -0.5.
                    // So if at the end of the exercise we would have a value
                    // for this combination of less than -0.5, we simply
                    // surrender and max out at -0.5.

                    eVal = std::max<bjgb::Types::Double>(eVal, -0.5);
                }

                expectedValue += pijk * eVal;

            } // end for k
        } // end for j
    } // end for i

    return expectedValue;
}

bool PlayerTableUtil::isSamePlayerRow(const PlayerTable& pta, int hia, const PlayerTable& ptb, int hib)
{
    assert(0 <= hia);
    assert(hia < bjgb::State::k_NUM_STATES);
    assert(0 <= hib);
    assert(hib < bjgb::State::k_NUM_STATES);

    for (int i = 1; i <= 10; ++i) {
        if (pta.exp(hia, i) != ptb.exp(hib, i)) {
            std::cout << "[FALSE: i = " << i << "]" << std::flush;

            return false; // RETURN
        }
    }

    return true;
}

bjgb::Types::Double PlayerTableUtil::mx(int hi, int cd, const PlayerTable& t, const PlayerTable& q)
{
    assert(0 <= hi);
    assert(hi < bjgb::State::k_NUM_STATES);
    assert(1 <= cd);
    assert(cd <= 10);

    assert((bjgb::State::e_HOV == hi) != bjgb::Types::isValid(t.exp(hi, cd)));
    // either it is OVER in which case the entry is invalid or vice versa
    assert(bjgb::Types::isValid(q.exp(hi, cd)));

    return bjgb::State::e_HOV == hi ? q.exp(hi, cd) : std::max<bjgb::Types::Double>(t.exp(hi, cd), q.exp(hi, cd));
}

bjgb::Types::Double
PlayerTableUtil::mx3(int hi, int cd, const PlayerTable& x, const PlayerTable& y, const PlayerTable& z)
{
    assert(0 <= hi);
    assert(hi < bjgb::State::k_NUM_STATES);
    assert(1 <= cd);
    assert(cd <= 10);
    assert(bjgb::Types::isValid(x.exp(hi, cd)));
    assert(bjgb::Types::isValid(y.exp(hi, cd)));
    assert(bjgb::Types::isValid(z.exp(hi, cd)));

    const bjgb::Types::Double temp = std::max<bjgb::Types::Double>(x.exp(hi, cd), y.exp(hi, cd));

    return std::max<bjgb::Types::Double>(temp, z.exp(hi, cd));
}

void PlayerTableUtil::populatePlayerTable(
    PlayerTable *pt, const PlayerTable& pxt, const PlayerTable& pht, const PlayerTable& pst, const bjgb::Rules& rules)
{
    // TBD this doc is not clear
    // The player table assumes that these are the first two cards (TBD
    // meaning what??).  Copy the maximum from the Splits, Hits, and Stands
    // tables.  Then transfer the split values.  This table alone is sufficient
    // to determine the odds of the game but we will still need separate
    // functions for strategy.
    //
    // Note that if the dealer peeks for BJ, we need to separate out the case
    // where the dealer gets blackjack and the adjusted shoe where he or she
    // doesn't.

    assert(pt);

    // Just copy the split values.

    for (int i = 1; i <= 10; ++i) {
        copyPlayerRow(pt, bjgb::State::pair(i), pxt, bjgb::State::pair(i));
    }

    // hard counts

    for (int i = 4; i <= 20; ++i) {
        copyPlayerRow(pt, bjgb::State::hard(i), pst, bjgb::State::hard(i));
        // copy the hard stand values

        for (int j = 1; j <= 10; ++j) {
            pt->exp(bjgb::State::hard(i), j) =
                std::max<bjgb::Types::Double>(pt->exp(bjgb::State::hard(i), j), pht.exp(bjgb::State::hard(i), j));

            if (rules.playerMayDoubleOnTheseTwoCards(i, false)) {
                std::cout << "[D: HARD i=" << i << ", j=" << j << "]\n";
                pt->exp(bjgb::State::hard(i), j) =
                    std::max<bjgb::Types::Double>(pt->exp(bjgb::State::hard(i), j), pxt.exp(bjgb::State::hard(i), j));
            }
        }
    }

    // soft counts

    for (int i = 2; i <= 10; ++i) {
        copyPlayerRow(pt, bjgb::State::soft(i), pst, bjgb::State::soft(i));
        // copy the soft stand values

        for (int j = 1; j <= 10; ++j) {
            pt->exp(bjgb::State::soft(i), j) =
                std::max<bjgb::Types::Double>(pt->exp(bjgb::State::soft(i), j), pht.exp(bjgb::State::soft(i), j));

            if (rules.playerMayDoubleOnTheseTwoCards(i, true)) {
                std::cout << "[D: SOFT i=" << i << ", j=" << j << "]\n";
                pt->exp(bjgb::State::soft(i), j) =
                    std::max<bjgb::Types::Double>(pt->exp(bjgb::State::soft(i), j), pxt.exp(bjgb::State::soft(i), j));
            }
        }
    }

    // blackjack

    copyPlayerRow(pt, bjgb::State::e_SBJ, pst, bjgb::State::e_SBJ);
    // copy the BJ values

    for (int j = 1; j <= 10; ++j) {
        pt->exp(bjgb::State::e_SBJ, j) =
            std::max<bjgb::Types::Double>(pt->exp(bjgb::State::e_SBJ, j), pht.exp(bjgb::State::e_SBJ, j));

        if (rules.playerMayDoubleOnTheseTwoCards(11, true)) {
            std::cout << "[D: BJ j=" << j << "]\n";
            pt->exp(bjgb::State::e_SBJ, j) =
                std::max<bjgb::Types::Double>(pt->exp(bjgb::State::e_SBJ, j), pxt.exp(bjgb::State::e_SBJ, j));
        }
    }
}

void PlayerTableUtil::populateP1tab(PlayerTable *p1,
                                    const PlayerTable& ps,
                                    const bjgb::Shoe& shoe,
                                    const bjgb::Rules& rules)
{
    assert(p1);

// 53 pair(10)
// 52 pair( 9)
// 51 pair( 8)
// 50 pair( 7)
// 49 pair( 6)
// 48 pair( 5)
// 47 pair( 4)
// 46 pair( 3)
// 45 pair( 2)
// 44 pair( 1)
#if 0
    for (int j = 10; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            p->exp(bjgb::State::pair(j), i) =
                                             eStand(q, i, j == 1 ? 12 : 2 * j);

            assert(p->exp(bjgb::State::pair(j), i) ==
                                 j >= 6 ? p->exp(bjgb::State::hard(2 * j), i)
                                        : p->exp(bjgb::State::hard(   16), i));
        }
    }
#endif

    for (int j = 10; j >= 1; --j) { // card value for each split hand
        for (int i = 1; i <= 10; ++i) {
            p1->exp(bjgb::State::pair(j), i) = eHit1(ps, i, 2 * j, j == 1, shoe);
        }
    }
    // Checked below.

    // ---------
    // 43 e_HOV  // cannot hit a hand that is OVer
    // N.A.

    // 42 hard(21)
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
    // 24 hard( 3)  // this has to be a single 3 after a split
    // 23 hard( 2)  // this has to be a single 2 after a split

    for (int j = 21; j >= 2; --j) {
        for (int i = 1; i <= 10; ++i) {
            p1->exp(bjgb::State::hard(j), i) = eHit1(ps, i, j, false, shoe);
        }
    }

    // --------
    // 22 unus(10)
    // 21 unus( 9)
    // 20 unus( 8)
    // 19 unus( 7)
    // 18 unus( 6)
    // 17 unus( 5)
    // 16 unus( 4)
    // 15 unus( 3)
    // 14 unus( 2)
    // 13 unus( 1)  // this is the first card of two and it is an Ace.

    for (int j = 10; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            p1->exp(bjgb::State::unus(j), i) = eHit1(ps, i, j, 1 == j, shoe);
        }

        std::cout << "[Z j = " << j << "]" << std::flush;

        assert(isSamePlayerRow(*p1, bjgb::State::unus(j), *p1, 1 == j ? bjgb::State::unus(j) : bjgb::State::hard(j)));
        // check 1 case below
    }

    // 12 e_HZR  // cannot hit one card only on no cards
    // N.A.

    // --------
    // 11 e_SBJ  // so treat this as an S11  (STUPID IS AS STUPID DOES)

    for (int i = 1; i <= 10; ++i) {
        p1->exp(bjgb::State::e_SBJ, i) = eHit1(ps, i, 11, true, shoe);
    }

    // 10 soft(11)
    //  9 soft(10)
    //  8 soft( 9)
    //  7 soft( 8)
    //  6 soft( 7)
    //  5 soft( 6)
    //  4 soft( 5)
    //  3 soft( 4)
    //  2 soft( 3)
    //  1 soft( 2)
    //  0 soft( 1)  // this is after a split Ace, which you *must* hit, no BJ

    for (int j = 11; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            p1->exp(bjgb::State::soft(j), i) = eHit1(ps, i, j, true, shoe);
        }
    }

    assert(isSamePlayerRow(*p1, bjgb::State::e_SBJ, *p1, bjgb::State::soft(11)));

    for (int j = 10; j >= 2; --j) { // card this is split (TBD meaning??)
        assert(isSamePlayerRow(*p1, bjgb::State::pair(j), *p1, bjgb::State::hard(2 * j)));
    }

    assert(isSamePlayerRow(*p1, bjgb::State::pair(1), *p1, bjgb::State::soft(2)));

    assert(isSamePlayerRow(*p1, bjgb::State::unus(1), *p1, bjgb::State::soft(1)));
}

void PlayerTableUtil::populatePdtab(PlayerTable *pd, const PlayerTable& p1)
{
    assert(pd);
    assert(bjgb::Types::isValid(p1.exp(bjgb::State::hard(20), 1)));
    assert(bjgb::Types::isValid(p1.exp(bjgb::State::hard(4), 9)));
    assert(bjgb::Types::isValid(p1.exp(bjgb::State::hard(11), 2)));
    assert(bjgb::Types::isValid(p1.exp(bjgb::State::hard(2), 10)));

    // 53 pair(10)
    // 52 pair( 9)
    // 51 pair( 8)
    // 50 pair( 7)
    // 49 pair( 6)
    // 48 pair( 5)
    // 47 pair( 4)
    // 46 pair( 3)
    // 45 pair( 2)
    // 44 pair( 1)
    // N.A.

    // ---------
    // 43 e_HOV
    // N.A.

    // 42 hard(21) // 3+ cards, not appropriate for a double
    // N.A.

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

    for (int j = 20; j >= 4; --j) {
        std::cout << "j=" << j << std::flush;
        for (int i = 1; i <= 10; ++i) {
            pd->exp(bjgb::State::hard(j), i) = eDouble(p1, i, j, false);
        }
    }

    // 24 hard(3)  // this has to be a single 3 after a split
    // 23 hard(2)  // this has to be a single 2 after a split
    // N.A.

    // --------
    // 22 unus(10)
    // 21 unus( 9)
    // 20 unus( 8)
    // 19 unus( 7)
    // 18 unus( 6)
    // 17 unus( 5)
    // 16 unus( 4)
    // 15 unus( 3)
    // 14 unus( 2)
    // 13 unus( 1)  // This is the first card of two and it is an Ace.
    // N.A.

    // 12 e_HZR  // cannot hit one card only on no cards
    // N.A.

    // --------
    // 11 e_SBJ  // so treat this as an S11  (STUPID IS AS STUPID DOES)

    for (int i = 1; i <= 10; ++i) {
        pd->exp(bjgb::State::e_SBJ, i) = eDouble(p1, i, 11, true);
    }

    // 10 soft(11)
    //  9 soft(10)
    //  8 soft( 9)
    //  7 soft( 8)
    //  6 soft( 7)
    //  5 soft( 6)
    //  4 soft( 5)
    //  3 soft( 4)
    //  2 soft( 3)
    //  1 soft( 2)

    for (int j = 11; j >= 2; --j) {
        std::cout << "J=" << j << std::flush;
        for (int i = 1; i <= 10; ++i) {
            pd->exp(bjgb::State::soft(j), i) = eDouble(p1, i, j, true);
        }
    }

    assert(isSamePlayerRow(*pd, bjgb::State::e_SBJ, *pd, bjgb::State::soft(11)));

    //  0 soft(1)  // this is after a split Ace, which you *must* hit, no BJ
    // N.A.
}

void PlayerTableUtil::populatePhtab(PlayerTable *ph,
                                    const PlayerTable& ps,
                                    const bjgb::Shoe& shoe,
                                    const bjgb::Rules& rules)
{
    assert(ph);

    // 53 pair(10)
    // 52 pair( 9)
    // 51 pair( 8)
    // 50 pair( 7)
    // 49 pair( 6)
    // 48 pair( 5)
    // 47 pair( 4)
    // 46 pair( 3)
    // 45 pair( 2)
    // 44 pair( 1)
    // N.A.

    // ---------
    // 43 e_HOV
    // N.A.

    // 42 hard(21)
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

    for (int j = 21; j >= 11; --j) {
        for (int i = 1; i <= 10; ++i) {
            ph->exp(bjgb::State::hard(j), i) = eHitN(*ph, ps, i, j, false, shoe);
        }
    }

    // 10 soft(11)
    //  9 soft(10)
    //  8 soft( 9)
    //  7 soft( 8)
    //  6 soft( 7)
    //  5 soft( 6)
    //  4 soft( 5)
    //  3 soft( 4)
    //  2 soft( 3)
    //  1 soft( 2)
    //  0 soft( 1)  // this is after a split Ace, which you *must* hit, no BJ

    for (int j = 11; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            ph->exp(bjgb::State::soft(j), i) = eHitN(*ph, ps, i, j, true, shoe);
        }
    }

    // 31 hard(10)
    // 30 hard( 9)
    // 29 hard( 8)
    // 28 hard( 7)
    // 27 hard( 6)
    // 26 hard( 5)
    // 25 hard( 4)
    // 24 hard( 3)  // this has to be a single 3 after a split
    // 23 hard( 2)  // this has to be a single 2 after a split

    for (int j = 10; j >= 2; --j) {
        for (int i = 1; i <= 10; ++i) {
            ph->exp(bjgb::State::hard(j), i) = eHitN(*ph, ps, i, j, false, shoe);
        }
    }

    // --------
    // 22 unus(10)
    // 21 unus( 9)
    // 20 unus( 8)
    // 19 unus( 7)
    // 18 unus( 6)
    // 17 unus( 5)
    // 16 unus( 4)
    // 15 unus( 3)
    // 14 unus( 2)
    // 13 unus( 1)  // This is the first card of two and it is an Ace.

    for (int j = 10; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            ph->exp(bjgb::State::unus(j), i) = eHitN(*ph, ps, i, j, 1 == j, shoe);
        }

        assert(isSamePlayerRow(*ph, bjgb::State::unus(j), *ph, 1 == j ? bjgb::State::soft(j) : bjgb::State::hard(j)));
    }

    // 12 e_HZR  // cannot hit one card only on no cards
    // N.A.

    // --------
    // 11 e_SBJ  // so treat this as an S11  (STUPID IS AS STUPID DOES)

    for (int i = 1; i <= 10; ++i) {
        ph->exp(bjgb::State::e_SBJ, i) = eHitN(*ph, ps, i, 11, true, shoe);
    }

    assert(isSamePlayerRow(*ph, bjgb::State::e_SBJ, *ph, bjgb::State::soft(11)));

    // 53 pair(10)
    // 52 pair( 9)
    // 51 pair( 8)
    // 50 pair( 7)
    // 49 pair( 6)
    // 48 pair( 5)
    // 47 pair( 4)
    // 46 pair( 3)
    // 45 pair( 2)
    // 44 pair( 1)

    for (int j = 10; j >= 1; --j) { // card value for each split hand
        for (int i = 1; i <= 10; ++i) {
            ph->exp(bjgb::State::pair(j), i) = eHitN(*ph, ps, i, 2 * j, 1 == j, shoe);
        }
    }

    for (int j = 10; j >= 2; --j) { // card this is split (TBD meaning??)
        assert(isSamePlayerRow(*ph, bjgb::State::pair(j), *ph, bjgb::State::hard(2 * j)));
    }

    assert(isSamePlayerRow(*ph, bjgb::State::pair(1), *ph, bjgb::State::soft(2)));
}

void PlayerTableUtil::populatePstab(PlayerTable *ps,
                                    const DealerTable& dt,
                                    const bjgb::Shoe& shoe,
                                    const bjgb::Rules& rules)
{
    assert(ps);

    // 53 pair(10)
    // 52 pair( 9)
    // 51 pair( 8)
    // 50 pair( 7)
    // 49 pair( 6)
    // 48 pair( 5)
    // 47 pair( 4)
    // 46 pair( 3)
    // 45 pair( 2)
    // 44 pair( 1)

    for (int j = 10; j >= 1; --j) {
        for (int i = 1; i <= 10; ++i) {
            ps->exp(bjgb::State::pair(j), i) = eStand(dt, i, 1 == j ? 12 : 2 * j);

            assert(ps->exp(bjgb::State::pair(j), i) == j >= 6 ? ps->exp(bjgb::State::hard(2 * j), i)
                                                              : ps->exp(bjgb::State::hard(16), i));
        }
    }

    // ---------
    // 43 e_HOV

    ps->setRow(bjgb::State::e_HOV, -1.0); // set all entries to simple loss

    // 42 hard(21)
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

    for (int j = 21; j >= 4; --j) {
        for (int i = 1; i <= 10; ++i) {
            ps->exp(bjgb::State::hard(j), i) = eStand(dt, i, j);
        }
    }

    // 24 hard(3)  // this has to be a 3 card after a split
    // 23 hard(2)  // this has to be a 2 card after a split
    // N.A.

    // --------
    // 22 unus(10)
    // 21 unus( 9)
    // 20 unus( 8)
    // 19 unus( 7)
    // 18 unus( 6)
    // 17 unus( 5)
    // 16 unus( 4)
    // 15 unus( 3)
    // 14 unus( 2)
    // 13 unus( 1)  // This is the first card of two and it is an Ace.
    // N.A.

    // 12 e_HZR  // cannot stand on no cards

    // --------
    // 11 e_SBJ

    ps->setRow(bjgb::State::e_SBJ, rules.playerBlackjackPayout());
    // set all entries blackjack

    ps->exp(bjgb::State::e_SBJ, 1) -= shoe.prob(10) * rules.playerBlackjackPayout();
    ps->exp(bjgb::State::e_SBJ, 10) -= shoe.prob(1) * rules.playerBlackjackPayout();

    // 10 soft(11)
    //  9 soft(10)
    //  8 soft( 9)
    //  7 soft( 8)
    //  6 soft( 7)
    //  5 soft( 6)
    //  4 soft( 5)
    //  3 soft( 4)
    //  2 soft( 3)
    //  1 soft( 2)

    for (int j = 11; j >= 2; --j) {
        for (int i = 1; i <= 10; ++i) {
            ps->exp(bjgb::State::soft(j), i) = eStand(dt, i, j + 10);
        }
    }

    //  0 soft(1)  // this is after a split Ace, which you *must* hit, no BJ
    // N.A.
}

void PlayerTableUtil::populatePxtab(PlayerTable *pxt,
                                    const PlayerTable& pdt,
                                    const PlayerTable& pht,
                                    const PlayerTable& p1t,
                                    const PlayerTable& pst,
                                    const bjgb::Shoe& shoe,
                                    const bjgb::Rules& rules)
{
    assert(pxt);

    // Copy over the values from the double table.

    for (int i = 4; i <= 20; ++i) { // there is no hard 2-card 21
        copyPlayerRow(pxt, bjgb::State::hard(i), pdt, bjgb::State::hard(i));
    }

    for (int i = 2; i <= 11; ++i) { // A+A, A+2, ... A+T (after a split Ace)
        copyPlayerRow(pxt, bjgb::State::soft(i), pdt, bjgb::State::soft(i));
    }

    copyPlayerRow(pxt, bjgb::State::e_SBJ, pdt, bjgb::State::e_SBJ);
    // This is a natural A + 10.

    for (int i = 2; i <= 20; i += 2) {
        int splitCard = i / 2;
        int maxSplits = rules.playerMaxNumHands() - 1; // typically 3

        for (int j = 1; j <= 10; ++j) {
            pxt->exp(bjgb::State::pair(splitCard), j) = eSplit(pdt, pht, p1t, pst, j, i, maxSplits, shoe, rules);
        }
    }
}

} // namespace bjgc
