// bjgb_shoe.h                                                        -*-C++-*-
#ifndef INCLUDED_BJGB_SHOE
#define INCLUDED_BJGB_SHOE

//@PURPOSE: Provide a value-semantic type that represents a blackjack shoe.
//
//@CLASSES:
//  bjgb::Shoe: value-semantic type representing a blackjack shoe
//
//@SEE_ALSO: TBD
//
//@DESCRIPTION: This component defines a value-semantic class, 'bjgb::Shoe',
// TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_rank.h>
#include <bjgb_types.h> // 'Double'

#include <cassert>
#include <cstdint>
#include <iosfwd>

namespace bjgb {

// ==========
// class Shoe
// ==========

class Shoe {
    // TBD class-level doc

    // DATA
    std::uint8_t d_rankCounts[1 + 10]; // 10 distinct ranks in blackjack;
                                       // unused 0th element is for
                                       // convenience of indexing

    int d_totalCards; // maintained as the sum of
                      // 'd_rankCounts[i]', 'i > 0'

    Types::Double d_prob[1 + 10]; // maintained as the probability of
                                  // each 'd_rankCounts[i]', 'i > 0'

  private:
    // PRIVATE MANIPULATORS
    void updateCache();
    // Private method that updates 'd_totalCards' whenever 'd_rankCounts'
    // changes.

  public:
    // CREATORS
    explicit Shoe(int numDecks = 6);
    // Create a shoe initially containing the optionally specified
    // 'numDecks'.  If 'numDecks' is not specified, the shoe will initially
    // contain 6 decks.  The behavior is undefined unless '1 <= numDecks'
    // and 'numDecks <= 8'.

    explicit Shoe(const int *rankCounts);
    // Create a shoe whose initial number of cards of each rank is provided
    // by the specified 'rankCounts' array.  'rankCounts[0]' provides the
    // number of aces, 'rankCounts[1]' the number of 2s, etc.  The
    // behavior is undefined unless '0 <= rankCounts[i] <= 32' for 'i < 9'
    // and '0 <= rankCounts[9] <= 128'.

    Shoe(const Shoe& original) = default;
    // Create a shoe having the same value as that of the specified
    // 'original' object.

    ~Shoe() = default;
    // Destroy this shoe object.

    // MANIPULATORS
    Shoe& operator=(const Shoe& rhs) = default;
    // Assign to this object the value of the specified 'rhs' shoe, and
    // return a reference providing modifiable access to this object.

    void setNumCardsOfEachRank(const int *rankCounts);
    // Set the current number of cards of each rank in this shoe as
    // provided by the specified 'rankCounts' array.  'rankCounts[0]'
    // provides the number of aces, 'rankCounts[1]' the number of 2s, etc.
    // The behavior is undefined unless '0 <= rankCounts[i] <= 32' for
    // 'i < 9' and '0 <= rankCounts[9] <= 128'.

    void setNumCardsOfRank(const Rank& rank, int count);
    // Set the current number of cards in this shoe having the specified
    // 'rank' to the specified 'count'.  The number of cards of other ranks
    // is not affected.  The behavior is undefined unless
    // '0 <= count <= 32' for 9s and below and '0 <= count <= 128' for 10s.

    // ACCESSORS
    int numCards(const Rank& rank) const;
    // Return the current number of cards in this shoe having the specified
    // 'rank'.

    int numCardsTotal() const;
    // Return the current number of cards of all ranks in this shoe.

    Types::Double prob(int rank) const;
    // TBD preferably this would take 'const Rank&'
    // TBD move this to bjgc::RankProb (or something)
    // Return the probability of receiving a card having the specified
    // 'rank' from this shoe.  The behavior is undefined unless '1 <= rank'
    // and 'rank <= 10'.
};

// FREE OPERATORS
bool operator==(const Shoe& lhs, const Shoe& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' shoes have the same value
// and 'false' otherwise.  Two 'Shoe' objects have the same value if the
// quantity of each respective rank is the same in each shoe.

bool operator!=(const Shoe& lhs, const Shoe& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' shoes do not have the
// same value and 'false' otherwise.  Two 'Shoe' objects do not have the
// same value if the quantity of some respective rank is different in each
// shoe.

std::ostream& operator<<(std::ostream& stream, const Shoe& shoe);
// Write the value of the specified 'shoe' object to the specified output
// 'stream', and return a reference to 'stream'.  Note that this
// human-readable format is not fully specified and can change without
// notice.

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// ----------
// class Shoe
// ----------

// MANIPULATORS
inline void Shoe::setNumCardsOfRank(const Rank& rank, int count)
{
    assert(0 <= count);
    assert(count <= (10 == rank.value() ? 128 : 32));

    d_rankCounts[rank.value()] = count;

    updateCache();
}

// ACCESSORS
inline int Shoe::numCards(const Rank& rank) const
{
    return d_rankCounts[rank.value()];
}

inline int Shoe::numCardsTotal() const
{
    return d_totalCards;
}

inline Types::Double Shoe::prob(int rank) const
{
    assert(1 <= rank);
    assert(rank <= 10);

    return d_prob[rank];
}

} // namespace bjgb

// FREE OPERATORS
inline bool bjgb::operator!=(const Shoe& lhs, const Shoe& rhs)
{
    return !(lhs == rhs);
}

#endif
