// bjgb_rank.h                                                        -*-C++-*-
#ifndef INCLUDED_BJGB_RANK
#define INCLUDED_BJGB_RANK

//@PURPOSE: Enumerate the distinct ranks (values) of cards in blackjack.
//
//@CLASSES:
//  bjgb::Rank: class representing the ranks of blackjack cards
//
//@DESCRIPTION: This component provides a class, 'bjgb::Rank', and its
// associated user-defined literal (UDL) operator that enumerate the ten
// distinct card ranks (or values) in blackjack.
//
/// Literals
///--------
//..
//  Name   Value
//  ----   --------------------------------------------
//   A_R     1 (Ace; alias for 1_R)
//   2_R     2
//   3_R     3
//   4_R     4
//   5_R     5
//   6_R     6
//   7_R     7
//   8_R     8
//   9_R     9
//   T_R    10 (Ten, Jack, Queen, King; alias for 10_R)
//..
//
// Iteration over the ranks is supported by member function 'operator++()',
// free 'operator<', and the 'end' class method.
//
/// Usage
///-----
// Iterate over the range of ranks and print each:
//..
//  for (Rank r = A_R; r != Rank::end(); ++r) {
//      if (r != A_R) {
//          std::cout << ' ';
//      }
//      std::cout << r;
//  }
//
//  std::cout << '\n';
//..
// The above prints the following to 'std::cout':
//..
//  A 2 3 4 5 6 7 8 9 T
//..

#include <cassert>
#include <iosfwd>

namespace bjgb {

class Rank;

// UDL OPERATORS
inline namespace literals {
inline namespace RankLiterals {
constexpr Rank operator""_R(unsigned long long value);
} // namespace RankLiterals
} // namespace literals

// ==========
// class Rank
// ==========

class Rank {
    // This class represents the ranks of cards in blackjack.  Instances of
    // this class can be created only via either the associated UDL operator,
    // '_R', or the 'end' class method.

    // DATA
    int d_value; // the range '[1 .. 10]' corresponds to A, 2, ..., T; 11 is
                 // used to represent the one-past-the-end value for iterating

    // FRIENDS
    friend constexpr Rank RankLiterals::operator""_R(unsigned long long);
    friend bool operator==(const Rank&, const Rank&);
    friend bool operator!=(const Rank&, const Rank&);
    friend bool operator<(const Rank&, const Rank&);

  private:
    // PRIVATE CREATORS
    explicit constexpr Rank(int value);
    // Create a rank object having the specified 'value'.  The behavior is
    // undefined unless '1 <= value' and 'value <= 11'.  Note that
    // 'Rank(11)' provides the value for 'Rank::end()'.

  public:
    // CLASS METHODS
    static Rank end();
    // Return a rank object having the one-past-the-end value.

    // CREATORS
    Rank() = delete;

    Rank(const Rank& original) = default;
    // Create a rank object having the same value as that of the specified
    // 'original' object.

    ~Rank() = default;
    // Destroy this rank object.

    // MANIPULATORS
    Rank& operator=(const Rank& rhs) = default;
    // Assign to this object the value of the specified 'rhs' rank, and
    // return a reference providing modifiable access to this object.

    Rank& operator++();
    // Increment this rank and return a non-'const' reference to this
    // object.  The behavior is undefined unless '*this < Rank::end()' on
    // entry.  Note that, for example:
    //..
    //  ++(5_R) == 6_R && ++(T_R) == Rank::end();
    //..

    // ACCESSORS
    int value() const;
    // Return the integral value of this rank.  The behavior is undefined
    // unless '*this != Rank::end()'.
};

// FREE OPERATORS
bool operator==(const Rank& lhs, const Rank& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' ranks have the same
// value, and 'false' otherwise.  Two 'Rank' objects have the same value if
// they correspond to the same UDL in the range '[A_R .. T_R]' or both have
// the same value as 'Rank::end()'.

bool operator!=(const Rank& lhs, const Rank& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' ranks do not have the
// same value, and 'false' otherwise.  Two 'Rank' objects do not have the
// same value if they correspond to different UDLs in the range
// '[A_R .. T_R]' or one of them, but not both, has the same value as
// 'Rank::end()'.

bool operator<(const Rank& lhs, const Rank& rhs);
// Return 'true' if the specified 'lhs' rank is less than the specified
// 'rhs' rank, and 'false' otherwise.  All ranks corresponding to UDLs in
// the range '[A_R .. T_R]' compare less than 'Rank::end()'.

std::ostream& operator<<(std::ostream& stream, const Rank& rank);
// Write the string representation of the specified 'rank' to the specified
// output 'stream' in a single-line format, and return a reference to
// 'stream'.

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ----------
// class Rank
// ----------

// PRIVATE CREATORS
inline constexpr Rank::Rank(int value): d_value(value)
{
    assert(1 <= value);
    assert(value <= 11);
}

// CLASS METHODS
inline Rank Rank::end()
{
    return Rank(11);
}

// MANIPULATORS
inline Rank& Rank::operator++()
{
    assert(d_value <= 10);

    ++d_value;

    return *this;
}

// ACCESSORS
inline int Rank::value() const
{
    assert(d_value <= 10);

    return d_value;
}

} // namespace bjgb

// FREE OPERATORS
inline bool bjgb::operator==(const Rank& lhs, const Rank& rhs)
{
    return lhs.d_value == rhs.d_value;
}

inline bool bjgb::operator!=(const Rank& lhs, const Rank& rhs)
{
    return lhs.d_value != rhs.d_value;
}

inline bool bjgb::operator<(const Rank& lhs, const Rank& rhs)
{
    return lhs.d_value < rhs.d_value;
}

// UDL OPERATORS
inline constexpr bjgb::Rank bjgb::RankLiterals::operator""_R(unsigned long long value)
{
    assert(1 <= value);
    assert(value <= 10);

    return Rank(value);
}

namespace bjgb {
inline namespace literals {
inline namespace RankLiterals {
constexpr Rank A_R = 1_R;
constexpr Rank T_R = 10_R;
} // namespace RankLiterals
} // namespace literals
} // namespace bjgb

#endif
