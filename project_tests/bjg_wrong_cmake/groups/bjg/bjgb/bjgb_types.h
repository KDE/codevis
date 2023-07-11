// bjgb_types.h                                                       -*-C++-*-
#ifndef INCLUDED_BJGB_TYPES
#define INCLUDED_BJGB_TYPES

//@PURPOSE: Provide a namespace for type aliases used in 'bjg'.
//
//@CLASSES:
//  bjgb::Types: namespace for 'bjg' type names
//
//@SEE_ALSO: TBD
//
//@DESCRIPTION: This component provides a namespace, 'bjgb::Types', for a set
// of 'typedef's used ubiquitously throughout the 'bjg' package group.
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

namespace bjgb {

// ============
// struct Types
// ============

struct Types {
    // TBD class-level doc

    // TYPES
    typedef long double Double; // TBD doc (need a better name?)

  public:
    // CLASS METHODS
    static bool isValid(Double value);
    // TBD function-level doc
    // TBD this function needs a new name and a new home
};

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ------------
// struct Types
// ------------

// CLASS METHODS
inline bool Types::isValid(Double value)
{
    return -2.0 <= value && value <= 2.0;
}

} // namespace bjgb

#endif
