// bjgc_dealertableutil.h                                             -*-C++-*-
#ifndef INCLUDED_BJGC_DEALERTABLEUTIL
#define INCLUDED_BJGC_DEALERTABLEUTIL

//@PURPOSE: Provide utilities operating on dealer probability tables.
//
//@CLASSES:
//  bjgc::DealerTableUtil: namespace for dealer-table utilities
//
//@SEE_ALSO: bjgc_playertableutil
//
//@DESCRIPTION: This component defines a utility 'struct',
// 'bjgc::DealerTableUtil', TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_shoe.h>

namespace bjgc {

class DealerTable;

// ======================
// struct DealerTableUtil
// ======================

struct DealerTableUtil {
    // This 'struct' provides a namespace for nonprimitive utility functions
    // that operate on dealer probability tables (of type 'bjgc::DealerTable').

    // CLASS METHODS
    static void adjustForBj(DealerTable *dst, const DealerTable& src);
    // Copy the specified 'src' dealer table to the specified 'dst' dealer
    // table and multiply all 'dst' table entries in the Ace through 10
    // (single-card) rows by '1 / (1 - z)' where 'z' is the probability
    // (per the 'src' table) of getting blackjack in the corresponding row.
    // Note that this adjustment does not apply in a casino where you can
    // lose double your bet when doubling down against a 10 or Ace dealer
    // up card.

    static void populate(DealerTable *table, const bjgb::Shoe& shoe, bool dealerStandsOnSoft17);
    // Populate the specified dealer 'table' based on the specified 'shoe'
    // and 'dealerStandsOnSoft17' flag.  If 'dealerStandsOnSoft17' is
    // 'true' then the rules in effect dictate that the dealer must stand
    // on a soft 17, and they must hit a soft 17 otherwise.
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ----------------------
// struct DealerTableUtil
// ----------------------

} // namespace bjgc

#endif
