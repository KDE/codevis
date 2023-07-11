// bjgb_dealercount.h                                                 -*-C++-*-
#ifndef INCLUDED_BJGB_DEALERCOUNT
#define INCLUDED_BJGB_DEALERCOUNT

//@PURPOSE: Enumerate the possible final counts of a dealer blackjack hand.
//
//@CLASSES:
//  bjgb::DealerCount: enumeration of the possible final dealer counts
//
//@SEE_ALSO: TBD
//
//@DESCRIPTION: This component defines an enumeration, 'bjgb::DealerCount', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <cassert>

namespace bjgb {

// ==================
// struct DealerCount
// ==================

struct DealerCount {
    // TBD class-level doc

    // TYPES
    enum Enum { e_C17, e_C18, e_C19, e_C20, e_C21, e_CBJ, e_COV };

    enum { k_NUM_FINAL_COUNTS = e_COV + 1 };

    // CLASS METHODS
    static constexpr int fini(int count);
    // Return the value of the enumerator in the range '[e_C17 .. e_C21]'
    // corresponding to the specified final dealer hand 'count'.  The
    // behavior is undefined unless '17 <= count' and 'count <= 21'.  Note
    // that this method maps integral values in the range '[17 .. 21]' to
    // values in the range '[e_C17 .. e_C21]'.
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ------------------
// struct DealerCount
// ------------------

// CLASS METHODS
inline constexpr int DealerCount::fini(int count)
{
    assert(17 <= count);
    assert(count <= 21);

    return count - 17 + e_C17;
}

} // namespace bjgb

#endif
