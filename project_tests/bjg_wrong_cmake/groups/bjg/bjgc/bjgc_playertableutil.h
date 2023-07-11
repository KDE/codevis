// bjgc_playertableutil.h                                             -*-C++-*-
#ifndef INCLUDED_BJGC_PLAYERTABLEUTIL
#define INCLUDED_BJGC_PLAYERTABLEUTIL

//@PURPOSE: Provide utilities operating on player expected-value tables.
//
//@CLASSES:
//  bjgc::PlayerTableUtil: utilities on tables of player expected values
//
//@SEE_ALSO: bjgc_dealertableutil
//
//@DESCRIPTION: This component defines a utility 'struct',
// 'bjgc::PlayerTableUtil', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgc_playertable.h>

#include <bjgc_dealertable.h>

#include <bjgb_rules.h>
#include <bjgb_shoe.h>
#include <bjgb_types.h> // 'Double'

// TBD most 'PlayerTableUtil' methods should probably be renamed

namespace bjgc {

// ======================
// struct PlayerTableUtil
// ======================

struct PlayerTableUtil {
    // TBD class-level doc

    // CLASS METHODS
    static void copyPlayerRow(PlayerTable *pta, int hia, const PlayerTable& ptb, int hib);
    // Copy the respective values of the row at the specified 'hib' hand
    // index in the specified 'ptb' player table to those in the row at the
    // specified 'hia' hand index in the specified 'pta' player table.  The
    // behavior is undefined unless '0 <= hia',
    // 'hia < bjgb::State::k_NUM_STATES', '0 <= hib', and
    // 'hib < bjgb::State::k_NUM_STATES'.

    static bjgb::Types::Double eDouble(const bjgc::PlayerTable& p1t, int uc, int hv, bool sf);
    // Return the expected value of doubling a hand given the specified
    // player table for a player hitting a hand just once, 'p1t', the
    // specified dealer up card, 'uc', the specified *minimum* value of the
    // hand, 'hv', and the specified 'sf' flag indicating whether 'hv' is a
    // soft count.  Note that not all casinos allow the player to double on
    // any two cards, e.g., some allow a double on 9, 10, or 11 only -- the
    // rules must be checked at the call sight.

    static bjgb::Types::Double eHit1(const PlayerTable& pst, int uc, int hv, bool sf, const bjgb::Shoe& shoe);
    // Return the expected value of a player's hand given the specified
    // player table for standing on any two-card value, 'pst', the
    // specified dealer up card, 'uc', the specified *minimum* value of the
    // hand, 'hv', the specified 'sf' flag indicating whether 'hv' is a
    // soft count, and the specified 'shoe'.

    static bjgb::Types::Double
    eHitN(const PlayerTable& pht, const PlayerTable& pst, int uc, int hv, bool sf, const bjgb::Shoe& shoe);
    // Return the expected value of a player's hand assuming they hit AT
    // LEAST once, given the specified current player table for hitting at
    // least once, 'pht', the specified player-stands table, 'pst', the
    // specified dealer up card, 'uc', the specified *minimum* value of the
    // hand, 'hv', the specified 'sf' flag indicating whether 'hv' is a
    // soft count, and the specified 'shoe'.

    static bjgb::Types::Double eSplit(const PlayerTable& pdt,
                                      const PlayerTable& pht,
                                      const PlayerTable& p1t,
                                      const PlayerTable& pst,
                                      int uc,
                                      int hv,
                                      int n,
                                      const bjgb::Shoe& shoe,
                                      const bjgb::Rules& rules);
    // Return the expected value of a player's hand (assuming it can be
    // split) given the specified expected-value tables for doubling,
    // 'pdt', hitting at least once, 'pht', hitting once only, 'p1t', and
    // 'standing', 'pst', and the specified up card, 'uc', the specified
    // current value of the player's hand, 'hv', the specified remaining
    // number of times the hand can be split (often at most 3 on the first
    // [non-recursive] call, but typically 1 for Aces), 'n', the specified
    // 'shoe', and the specified 'rules'.  Note that, with each split, the
    // bet is doubled.  Also note that the 'shoe' is not currently affected
    // by repeated splits.  (TBD We might consider reducing the number of
    // the split card for smaller shoes.)  Also note that the rule of 1
    // card to a split Ace is separate from being allowed to re-split Aces;
    // hence, we need to pass 'rules' into this function and not rely
    // solely on the value of 'n', but it is up to the caller to inspect
    // the 'rules' and decide how many splits are allowed for cards: I.e.,
    // we pass in 1 less than the maximum number of allowed hands.

    static bjgb::Types::Double eStand(const DealerTable& t, int uc, int hv);
    // Return the expected value of a player's hand given the specified
    // dealer table, 't', the specified dealer up card, 'uc', and the
    // specified player hard-count value, 'hv'.

    static bjgb::Types::Double
    evalShoeImp(const bjgb::Shoe& shoe, const PlayerTable& pt, const PlayerTable& pat, const bjgb::Rules& rules);
    // Return the expected value of the specified 'shoe', given the
    // specified (unadjusted) player table, 'pt', the specified adjusted
    // player table, 'pat', and the specified 'rules'.

    static bool isSamePlayerRow(const PlayerTable& pta, int hia, const PlayerTable& ptb, int hib);
    // Return 'true' if the row at the specified 'hia' hand index in the
    // specified 'pta' player table holds the same values as the row at the
    // specified 'hib' hand index in the specified 'ptb' player table, and
    // 'false' otherwise.  The behavior is undefined unless '0 <= hia',
    // 'hia < bjgb::State::k_NUM_STATES', '0 <= hib', and
    // 'hib < bjgb::State::k_NUM_STATES'.

    static bjgb::Types::Double mx(int hi, int cd, const PlayerTable& t, const PlayerTable& q);
    // TBD the 'const PlayerTable&' arguments should be passed first
    // Return the maximum of the two entries in the specified 't' and 'q'
    // player tables that are indexed by the specified 'hi' hand index and
    // 'cd' raw card value.  The behavior is undefined unless '0 <= hi',
    // 'hi < bjgb::State::k_NUM_STATES', '1 <= cd', 'cd <= 10',
    // '(bjgb::State::e_HOV == hi) != bjgb::Types::isValid(t.exp(hi, cd))'
    // and 'bjgb::Types::isValid(q.exp(hi, cd))'.

    static bjgb::Types::Double mx3(int hi, int cd, const PlayerTable& x, const PlayerTable& y, const PlayerTable& z);
    // TBD the 'const PlayerTable&' arguments should be passed first
    // Return the maximum of the three entries in the specified 'x', 'y',
    // and 'z' player tables that are indexed by the specified 'hi' hand
    // index and 'cd' raw card value.  The behavior is undefined unless
    // '0 <= hi', 'hi < bjgb::State::k_NUM_STATES', '1 <= cd', 'cd <= 10',
    // 'bjgb::Types::isValid(x.exp(hi, cd))',
    // 'bjgb::Types::isValid(y.exp(hi, cd))', and
    // 'bjgb::Types::isValid(z.exp(hi, cd))'.

    static void populatePlayerTable(PlayerTable *pt,
                                    const PlayerTable& pxt,
                                    const PlayerTable& pht,
                                    const PlayerTable& pst,
                                    const bjgb::Rules& rules);
    // Populate the specified player table, 'pt', given the specified
    // player-splits table, 'pxt', the specified player-hits-many table,
    // 'pht', the specified player-stands table, 'pst', and the specified
    // 'rules'.

    static void populateP1tab(PlayerTable *p1, const PlayerTable& ps, const bjgb::Shoe& shoe, const bjgb::Rules& rules);
    // Populate the specified player-hits-once table, 'p1', assuming that
    // the player always hits exactly once, given the specified
    // player-stands table, 'ps', the specified 'shoe', and the specified
    // 'rules'.

    static void populatePdtab(PlayerTable *pd, const PlayerTable& p1);
    // Populate the specified player-doubles table, 'pd', assuming that the
    // player is allowed to double the hand, given the specified
    // player-hits-once table, 'p1'.  Valid hands to double are 'e_SBJ',
    // S11-S02, and H20-H04.
    // TBD not sure what this means:
    //  Note that we will need to pass in an adjusted table unless the
    //  rules allow for a doubled hand to lose more than unit value.
    //  Consult the rules.

    static void populatePhtab(PlayerTable *ph, const PlayerTable& ps, const bjgb::Shoe& shoe, const bjgb::Rules& rules);
    // Populate the specified player-hits-many table, 'ph', assuming that
    // the player always hits while it is to the player's advantage to do
    // so, given the specified player-stands table, 'ps', the specified
    // 'shoe', and the specified 'rules'.

    static void populatePstab(PlayerTable *ps, const DealerTable& dt, const bjgb::Shoe& shoe, const bjgb::Rules& rules);
    // Populate the specified player-stands table, 'ps', assuming that the
    // player always stands, given the specified dealer table, 'dt', the
    // specified 'shoe', and the specified 'rules'.

    static void populatePxtab(PlayerTable *pxt,
                              const PlayerTable& pdt,
                              const PlayerTable& pht,
                              const PlayerTable& p1t,
                              const PlayerTable& pst,
                              const bjgb::Shoe& shoe,
                              const bjgb::Rules& rules);
    // Populate the specified player-splits table, 'pxt', assuming that the
    // player splits whenever it is allowed to do so, given the specified
    // player-doubles table, 'pdt', the specified player-hits-many table,
    // 'pht', the specified player-hits-once table, 'p1t', the specified
    // player-stands table, 'pst', the specified 'shoe', and the specified
    // 'rules'.
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct PlayerTableUtil
// ----------------------

} // namespace bjgc

#endif
