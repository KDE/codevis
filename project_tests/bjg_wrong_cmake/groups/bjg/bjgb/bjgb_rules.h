// bjgb_rules.h                                                       -*-C++-*-
#ifndef INCLUDED_BJGB_RULES
#define INCLUDED_BJGB_RULES

//@PURPOSE: Provide a value-semantic type to represent the rules of blackjack.
//
//@CLASSES:
//  bjgb::Rules: value-semantic type representing blackjack rules
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component defines a value-semantic class, 'bjgb::Rules',
// TBD
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
// TBD

#include <bjgb_state.h>
#include <bjgb_types.h> // 'Double'

#include <cassert>
#include <iosfwd>

namespace bjgb {

// ===========
// class Rules
// ===========

class Rules {
    // This value-semantic attribute type represents the rules of blackjack.

    // DATA
    bool d_dealerStandsOnSoft17; // dealer must stand on soft 17

    bool d_playerMaySurrender; // player may surrender on any two
                               // cards (to an A or T dealer up card)

    bool d_oneCardToSplitAce; // player gets one card to a split Ace

    bool d_playerMayResplitAces; // player may resplit Aces

    bool d_playerCanLoseDouble; // player can lose double when
                                // doubling down against a 10 or Ace
                                // dealer up card

    bool d_playerMayDoubleAny2Cards; // player may double down on any
                                     // 2-card hand; if 'false', check
                                     // 'd_doubleableHand' array

    // TBD this needs to be rethought; bjgb_rule should not depend on
    // bjgb_state nor is populating this array convenient for the client
    bool d_doubleableHand[State::k_NUM_STATES];
    // array of (2-card) hands for which
    // player may double down; should be
    // queried when
    // 'd_playerMayDoubleAny2Cards' is
    // 'false'

    int d_playerMaxNumHands; // maximum number of player hands; one
                             // more than maximum splits allowed

    Types::Double d_playerBjPayout; // blackjack payout

    // FRIENDS
    friend bool operator==(const Rules&, const Rules&);
    friend std::ostream& operator<<(std::ostream&, const Rules&);

  public:
    // CREATORS
    Rules();
    // Create a 'Rules' object and initialize it with the rules in effect
    // on the high-rollers' tables at the casinos in Atlantic City, New
    // Jersey.

    Rules(const Rules& original) = default;
    // Create a 'Rules' object having the same value as that of the
    // specified 'original' object.

    // MANIPULATORS
    Rules& operator=(const Rules& rhs) = default;
    // Assign to this object the value of the specified 'rhs' object, and
    // return a reference providing modifiable access to this object.

    void reset();
    // Reset this object to the default-constructed state.

    // Dealer Rules

    void setDealerStandsOnSoft17(bool value);
    // Set the "DealerStandsOnSoft17" attribute of this 'Rules' object to
    // have the specified 'value'.

    // Player Rules

    void setPlayerBlackjackPayout(Types::Double value);
    // Set the "PlayerBlackjackPayout" attribute of this 'Rules' object to
    // have the specified 'value'.  The behavior is undefined unless
    // '1.0 <= value';

    void setPlayerCanLoseDouble(bool value);
    // Set the "PlayerCanLoseDouble" attribute of this 'Rules' object to
    // have the specified 'value'.

    void setPlayerMayDoubleOnAnyTwoCards(bool value);
    // Set the "PlayerMayDoubleOnAnyTwoCards" attribute of this 'Rules'
    // object to have the specified 'value'.

    void setPlayerMayDoubleOnTheseTwoCards(const bool *doubleableHand);
    // Set the "PlayerMayDoubleOnTheseTwoCards" attribute of this 'Rules'
    // object to have the contents of the specified 'doubleableHand'
    // array.

    void setPlayerMayResplitAces(bool value);
    // Set the "PlayerMayResplitAces" attribute of this 'Rules' object to
    // have the specified 'value'.

    void setPlayerMaySurrender(bool value);
    // Set the "PlayerMaySurrender" attribute of this 'Rules' object to
    // have the specified 'value'.

    void setPlayerGetsOneCardOnlyToSplitAce(bool value);
    // Set the "PlayerGetsOneCardOnlyToSplitAce" attribute of this 'Rules'
    // object to have the specified 'value'.

    void setPlayerMaxNumHands(int value);
    // Set the "PlayerMaxNumHands" attribute of this 'Rules' object to have
    // the specified 'value'.  The behavior is undefined unless
    // '0 > value'.

    // ACCESSORS

    // Dealer Rules

    bool dealerStandsOnSoft17() const;
    // Return the value of the "DealerStandsOnSoft17" attribute of this
    // 'Rules' object.

    // Player Rules

    Types::Double playerBlackjackPayout() const;
    // Return the value of the "PlayerBlackjackPayout" attribute of this
    // 'Rules' object.

    bool playerCanLoseDouble() const;
    // Return the value of the "PlayerCanLoseDouble" attribute of this
    // 'Rules' object.

    bool playerMayDoubleOnAnyTwoCards() const;
    // Return the value of the "PlayerMayDoubleOnAnyTwoCards" attribute of
    // this 'Rules' object.

    bool playerMayDoubleOnTheseTwoCards(int handValue, bool isSoftCount) const;
    // Return 'true' if the "PlayerMayDoubleOnTheseTwoCards" attribute of
    // this 'Rules' object indicates that a player may double on a 2-card
    // hand having the specified 'handValue', and 'false' otherwise.  If
    // the specified 'isSoftCount' flag is 'true' then 'handValue' is
    // interpreted as a soft count, otherwise 'handValue' is interpreted as
    // a hard count.  The behavior is undefined unless '2 <= handValue',
    // 'handValue <= 20', 'handValue >= 4 || isSoftCount', and
    // 'handValue <= 11 || !isSoftCount'.  Note that this method need not
    // be called if 'playerMayDoubleOnAnyTwoCards' returns 'true'.

    bool playerMayResplitAces() const;
    // Return the value of the "PlayerMayResplitAces" attribute of this
    // 'Rules' object.

    bool playerMaySurrender() const;
    // Return the value of the "PlayerMaySurrender" attribute of this
    // 'Rules' object.

    bool playerGetsOneCardOnlyToSplitAce() const;
    // Return the value of the "PlayerGetsOneCardOnlyToSplitAce" attribute
    // of this 'Rules' object.

    int playerMaxNumHands() const;
    // Return the value of the "PlayerMaxNumHands" attribute of this
    // 'Rules' object.
};

// FREE OPERATORS
bool operator==(const Rules& lhs, const Rules& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' 'Rules' objects have the
// same value and 'false' otherwise.  Two 'Rules' objects have the same
// value if each of their attributes (respectively) have the same value.

bool operator!=(const Rules& lhs, const Rules& rhs);
// Return 'true' if the specified 'lhs' and 'rhs' 'Rules' objects do not
// have the same value and 'false' otherwise.  Two 'Rules' objects do not
// have the same value if any of their attributes (respectively) do not
// have the same value.

std::ostream& operator<<(std::ostream& stream, const Rules& rules);
// Write the value of the specified 'rules' object to the specified output
// 'stream', and return a reference to 'stream'.  Note that this
// human-readable format is not fully specified and can change without
// notice.

// ============================================================================
//                              INLINE DEFINITIONS
// ============================================================================

// -----------
// class Rules
// -----------

// MANIPULATORS

// Dealer Rules

inline void Rules::setDealerStandsOnSoft17(bool value)
{
    d_dealerStandsOnSoft17 = value;
}

// Player Rules

inline void Rules::setPlayerBlackjackPayout(Types::Double value)
{
    assert(1.0 <= value);

    d_playerBjPayout = value;
}

inline void Rules::setPlayerCanLoseDouble(bool value)
{
    d_playerCanLoseDouble = value;
}

inline void Rules::setPlayerMayDoubleOnAnyTwoCards(bool value)
{
    d_playerMayDoubleAny2Cards = value;
}

inline void Rules::setPlayerMayResplitAces(bool value)
{
    d_playerMayResplitAces = value;
}

inline void Rules::setPlayerMaySurrender(bool value)
{
    d_playerMaySurrender = value;
}

inline void Rules::setPlayerGetsOneCardOnlyToSplitAce(bool value)
{
    d_oneCardToSplitAce = value;
}

inline void Rules::setPlayerMaxNumHands(int value)
{
    assert(0 < value);

    d_playerMaxNumHands = value;
}

// ACCESSORS

// Dealer Rules

inline bool Rules::dealerStandsOnSoft17() const
{
    return d_dealerStandsOnSoft17;
}

// Player Rules

inline Types::Double Rules::playerBlackjackPayout() const
{
    return d_playerBjPayout;
}

inline bool Rules::playerCanLoseDouble() const
{
    return d_playerCanLoseDouble;
}

inline bool Rules::playerMayDoubleOnAnyTwoCards() const
{
    return d_playerMayDoubleAny2Cards;
}

inline bool Rules::playerMayResplitAces() const
{
    return d_playerMayResplitAces;
}

inline bool Rules::playerMaySurrender() const
{
    return d_playerMaySurrender;
}

inline bool Rules::playerGetsOneCardOnlyToSplitAce() const
{
    return d_oneCardToSplitAce;
}

inline int Rules::playerMaxNumHands() const
{
    return d_playerMaxNumHands;
}

} // namespace bjgb

// FREE OPERATORS
inline bool bjgb::operator!=(const Rules& lhs, const Rules& rhs)
{
    return !(lhs == rhs);
}

#endif
