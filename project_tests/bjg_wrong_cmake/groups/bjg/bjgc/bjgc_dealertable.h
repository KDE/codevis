// bjgc_dealertable.h                                                 -*-C++-*-
#ifndef INCLUDED_BJGC_DEALERTABLE
#define INCLUDED_BJGC_DEALERTABLE

//@PURPOSE: Provide a mechanism to tabulate dealer probabilities.
//
//@CLASSES:
//  bjgc::DealerTable: table of dealer probabilities
//
//@SEE_ALSO: bjgb_state, bjgc_playertable
//
//@DESCRIPTION: This component defines a mechanism, 'bjgc::DealerTable', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_dealercount.h> // 'k_NUM_FINAL_COUNTS'
#include <bjgb_state.h> // 'k_NUM_STATES'
#include <bjgb_types.h> // 'Double'

#include <cassert>
#include <iosfwd>

namespace bjgc {

// =================
// class DealerTable
// =================

class DealerTable {
    // This class defines a mechanism for tabulating blackjack dealer odds.

    // DATA
    bjgb::Types::Double d_prob[bjgb::State::k_NUM_STATES][bjgb::DealerCount::k_NUM_FINAL_COUNTS];
    // table of dealer probabilities; indexing: [hand][final count]

  public:
    // CREATORS
    DealerTable();
    // Create a dealer table and initialize all entries with a value that
    // is distinct from any valid dealer probability.  The initial value of
    // the entries is implementation defined.

    DealerTable(const DealerTable&) = delete;

    ~DealerTable() = default;

    // MANIPULATORS
    DealerTable& operator=(const DealerTable&) = delete;

    constexpr bjgb::Types::Double& prob(int hand, int count);
    // Return a non-'const' reference to the entry in this dealer table
    // that is indexed by the specified 'hand' state and final 'count'.
    // The behavior is undefined unless '0 <= hand',
    // 'hand < bjgb::State::k_NUM_STATES', '0 <= count', and
    // 'count < bjgb::DealerCount::k_NUM_FINAL_COUNTS'.

    void clearRow(int hand);
    // Set all entries of the row in this dealer table that is indexed by
    // the specified 'hand' state to 0.0.  The behavior is undefined unless
    // '0 <= hand' and 'hand < bjgb::State::k_NUM_STATES'.

    void reset();
    // Reset this dealer table to the default constructed state.

    // ACCESSORS
    constexpr const bjgb::Types::Double& prob(int hand, int count) const;
    // Return a 'const' reference to the entry in this dealer table that is
    // indexed by the specified 'hand' state and final 'count'.  The
    // behavior is undefined unless '0 <= hand',
    // 'hand < bjgb::State::k_NUM_STATES', '0 <= count', and
    // 'count < bjgb::DealerCount::k_NUM_FINAL_COUNTS'.
};

// FREE OPERATORS
std::ostream& operator<<(std::ostream& stream, const DealerTable& table);
// Write the value of the specified dealer probabilities 'table' to the
// specified output 'stream', and return a reference to 'stream'.  Note
// that this human-readable format is not fully specified and can change
// without notice.

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// -----------------
// class DealerTable
// -----------------

// CREATORS
inline DealerTable::DealerTable()
{
    reset();
}

// MANIPULATORS
inline constexpr bjgb::Types::Double& DealerTable::prob(int hand, int count)
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);
    assert(0 <= count);
    assert(count < bjgb::DealerCount::k_NUM_FINAL_COUNTS);

    return d_prob[hand][count];
}

// ACCESSORS
inline constexpr const bjgb::Types::Double& DealerTable::prob(int hand, int count) const
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);
    assert(0 <= count);
    assert(count < bjgb::DealerCount::k_NUM_FINAL_COUNTS);

    return d_prob[hand][count];
}

} // namespace bjgc

#endif
