// bjgb_shoeutil.h                                                    -*-C++-*-
#ifndef INCLUDED_BJGB_SHOEUTIL
#define INCLUDED_BJGB_SHOEUTIL

//@PURPOSE: Provide utilities operating on blackjack shoes.
//
//@CLASSES:
//  bjgb::ShoeUtil: namespace for utilities on 'bjgb::Shoe'
//
//@SEE_ALSO: bjgb_shoe
//
//@DESCRIPTION: This component defines a utility 'struct', 'bjgb::ShoeUtil',
// TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_shoe.h>
#include <bjgb_types.h>

namespace bjgb {

// ===============
// struct ShoeUtil
// ===============

struct ShoeUtil {
    // This 'struct' provides a namespace for utilities that operate on
    // 'bjgb::Shoe' objects.

  public:
    // CLASS METHODS
    static bool isUnused(const Shoe& shoe);
    // Return 'true' if the specified 'shoe' is unused (e.g., since the
    // last shuffle), and 'false' otherwise.  Note that this method might
    // return false positives since it is possible that a shoe that
    // originally contained 8 decks could be used up evenly during the
    // course of a game to contain exactly the number of cards in 6 decks.
    // TBD this method seems dubious

    static void setTenRichness(Shoe *shoe, int n);
    // Set the ten-richness of the specified 'shoe' to the specified 'n'.
    // The behavior is undefined unless 'n <= 9'.  The ten-richness of a
    // shoe is a measure of the relative abundance ('+n') or scarcity
    // ('-n') of ten cards in a shoe as compared to the total number of
    // cards and satisfies the proportion '4 / (13 - n)'.

    static Types::Double tenRichness(const Shoe& shoe);
    // Return the ten-richness of the specified 'shoe'.  See
    // 'setTenRichness' for further information.
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ---------------
// struct ShoeUtil
// ---------------

} // namespace bjgb

#endif
