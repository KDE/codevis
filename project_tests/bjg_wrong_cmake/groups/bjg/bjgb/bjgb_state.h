// bjgb_state.h                                                       -*-C++-*-
#ifndef INCLUDED_BJGB_STATE
#define INCLUDED_BJGB_STATE

#include <bjgb_export.h>

//@PURPOSE: Enumerate the unique states of a blackjack hand.
//
//@CLASSES:
//  bjgb::State: enumeration of the unique states of a blackjack hand
//
//@SEE_ALSO: TBD
//
//@DESCRIPTION: This component defines an enumeration, 'bjgb::State', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <cassert>

namespace bjgb {

// ============
// struct State
// ============

struct BJGB_EXPORT State {
    // TBD class-level doc

    // TYPES
    enum Enum {
        // Soft counts (TBD explain 'e_S01')
        e_S01,
        e_S02,
        e_S03,
        e_S04,
        e_S05,
        e_S06,
        e_S07,
        e_S08,
        e_S09,
        e_S10,
        e_S11,

        // BlackJack ("natural" 21)
        e_SBJ,

        // 0-card hand (ZeRo count)
        e_HZR,

        // 1-card hands; lone Ace is soft ('S_'), other cards are hard ('H_')
        e_S_A,
        e_H_2,
        e_H_3,
        e_H_4,
        e_H_5,
        e_H_6,
        e_H_7,
        e_H_8,
        e_H_9,
        e_H_T,

        // Hard counts (TBD explain 'e_H02', 'e_H03'; bogus states?)
        e_H02,
        e_H03,
        e_H04,
        e_H05,
        e_H06,
        e_H07,
        e_H08,
        e_H09,
        e_H10,
        e_H11,
        e_H12,
        e_H13,
        e_H14,
        e_H15,
        e_H16,
        e_H17,
        e_H18,
        e_H19,
        e_H20,
        e_H21,

        // OVer (busted)
        e_HOV,

        // Pairs of identical cards (opportunity for player split)
        e_PAA,
        e_P22,
        e_P33,
        e_P44,
        e_P55,
        e_P66,
        e_P77,
        e_P88,
        e_P99,
        e_PTT
    };

    enum { k_NUM_STATES = e_PTT + 1 };

    // CLASS METHODS
    static constexpr int hard(int count);
    // Return the value of the enumerator in the range '[e_H02 .. e_H21]'
    // corresponding to the specified *hard* 'count'.  The behavior is
    // undefined unless '2 <= count' and 'count <= 21'.  Note that this
    // method maps integral values in the range '[2 .. 21]' to values in
    // the range '[e_H02 .. e_H21]'.

    static constexpr int soft(int count);
    // Return the value of the enumerator in the range '[e_S01 .. e_S11]'
    // corresponding to the specified *soft* 'count'.  The behavior is
    // undefined unless '1 <= count' and 'count <= 11'.  Note that this
    // method maps integral values in the range '[1 .. 11]' to values in
    // the range '[e_S01 .. e_S11]'.

    static constexpr int pair(int value);
    // Return the value of the enumerator in the range '[e_PAA .. e_PTT]'
    // corresponding to the specified card 'value'.  The behavior is
    // undefined unless '1 <= value' and 'value <= 10'.  Note that this
    // method maps integral values in the range '[1 .. 10]' to values in
    // the range '[e_PAA .. e_PTT]'.

    static constexpr int unus(int value);
    // Return the value of the enumerator in the range '[e_S_A .. e_H_T]'
    // corresponding to the specified card 'value'.  The behavior is
    // undefined unless '1 <= value' and 'value <= 10'.  Note that this
    // method maps integral values in the range '[1 .. 10]' to values in
    // the range '[e_S_A .. e_H_T]'.

    static const char *stateId2String(int stateId);
    // Return the compact string representation corresponding to the
    // specified 'stateId'.  The behavior is undefined unless
    // '0 <= stateId' and 'stateId < k_NUM_STATES'.  Note that this method
    // provides a unique 2-character "label" for each state to distinguish
    // them in output.
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ------------
// struct State
// ------------

// CLASS METHODS
inline constexpr int State::hard(int count)
{
    assert(2 <= count);
    assert(count <= 21);

    return count - 2 + e_H02;
}

inline constexpr int State::soft(int count)
{
    assert(1 <= count);
    assert(count <= 11);

    return count - 1 + e_S01;
}

inline constexpr int State::pair(int value)
{
    assert(1 <= value);
    assert(value <= 10);

    return value - 1 + e_PAA;
}

inline constexpr int State::unus(int value)
{
    assert(1 <= value);
    assert(value <= 10);

    return value - 1 + e_S_A;
}

} // namespace bjgb

#endif
