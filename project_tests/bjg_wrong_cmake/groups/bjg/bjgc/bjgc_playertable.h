// bjgc_playertable.h                                                 -*-C++-*-
#ifndef INCLUDED_BJGC_PLAYERTABLE
#define INCLUDED_BJGC_PLAYERTABLE

//@PURPOSE: Provide a mechanism to tabulate player expected values.
//
//@CLASSES:
//  bjgc::PlayerTable: table of player expected values
//
//@SEE_ALSO: bjgb_state, bjgc_dealertable
//
//@DESCRIPTION: This component defines a mechanism, 'bjgc::PlayerTable', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_state.h> // 'k_NUM_STATES'
#include <bjgb_types.h> // 'Double'

#include <iosfwd>

namespace bjgc {

// =================
// class PlayerTable
// =================

class PlayerTable {
    // This class defines a mechanism for tabulating blackjack player expected
    // values.

  public:
    // CLASS DATA
    static const int k_NUM_CARD_VALUES = 10; // number of card values

  private:
    // DATA
    bjgb::Types::Double d_exp[bjgb::State::k_NUM_STATES][1 + k_NUM_CARD_VALUES];
    // table of player expected values; indexing: [hand][card value];
    // 0th column is not used

  public:
    // CREATORS
    PlayerTable();
    // Create a player table and initialize all entries with a value that
    // is distinct from any valid player expected value.  The initial value
    // of the entries is implementation defined.

    PlayerTable(const PlayerTable&) = delete;

    ~PlayerTable() = default;

    // MANIPULATORS
    PlayerTable& operator=(const PlayerTable&) = delete;

    constexpr bjgb::Types::Double& exp(int hand, int value);
    // Return a non-'const' reference to the entry in this player table
    // that is indexed by the specified 'hand' state and card 'value'.  The
    // behavior is undefined unless '0 <= hand',
    // 'hand < bjgb::State::k_NUM_STATES', '1 <= value', and 'value <= 10'.

    void clearRow(int hand);
    // Set all entries of the row in this player table that is indexed by
    // the specified 'hand' state to 0.0.  The behavior is undefined unless
    // '0 <= hand' and 'hand < bjgb::State::k_NUM_STATES'.

    void reset();
    // Reset this player table to the default constructed state.

    void setRow(int hand, bjgb::Types::Double value);
    // Set all entries of the row in this player table that is indexed by
    // the specified 'hand' state to the specified 'value'.  The behavior
    // is undefined unless '0 <= hand' and
    // 'hand < bjgb::State::k_NUM_STATES'.

    // ACCESSORS
    constexpr const bjgb::Types::Double& exp(int hand, int value) const;
    // Return a 'const' reference to the entry in this player table that is
    // indexed by the specified 'hand' state and card 'value'.  The
    // behavior is undefined unless '0 <= hand',
    // 'hand < bjgb::State::k_NUM_STATES', '1 <= value', and 'value <= 10'.
};

// FREE OPERATORS
std::ostream& operator<<(std::ostream& stream, const PlayerTable& table);
// Write the value of the specified player expected-values 'table' to the
// specified output 'stream', and return a reference to 'stream'.  Note
// that this human-readable format is not fully specified and can change
// without notice.

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// -----------------
// class PlayerTable
// -----------------

// CREATORS
inline PlayerTable::PlayerTable()
{
    reset();
}

// MANIPULATORS
inline constexpr bjgb::Types::Double& PlayerTable::exp(int hand, int value)
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);
    assert(1 <= value);
    assert(value <= k_NUM_CARD_VALUES);

    return d_exp[hand][value];
}

// ACCESSORS
inline constexpr const bjgb::Types::Double& PlayerTable::exp(int hand, int value) const
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);
    assert(1 <= value);
    assert(value <= k_NUM_CARD_VALUES);

    return d_exp[hand][value];
}

} // namespace bjgc

#endif
