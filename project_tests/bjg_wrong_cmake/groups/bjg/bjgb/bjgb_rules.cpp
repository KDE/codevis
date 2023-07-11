// bjgb_rules.cpp                                                     -*-C++-*-
#include <bjgb_rules.h>

#include <algorithm> // 'fill_n', 'swap'
#include <ostream>

namespace bjgb {

// -----------
// class Rules
// -----------

// CREATORS
Rules::Rules():
    d_dealerStandsOnSoft17(true),
    d_playerMaySurrender(false) // TBD late surrender is allowed; why 'false'?
    ,
    d_oneCardToSplitAce(true),
    d_playerMayResplitAces(false),
    d_playerCanLoseDouble(false),
    d_playerMayDoubleAny2Cards(true),
    d_playerMaxNumHands(4),
    d_playerBjPayout(1.5)
{
    std::fill_n(d_doubleableHand, sizeof d_doubleableHand, true);
}

// MANIPULATORS
void Rules::reset()
{
    Rules temp;

    using std::swap;
    swap(temp, *this);
}

// Player Rules

void Rules::setPlayerMayDoubleOnTheseTwoCards(const bool *doubleableHand)
{
    assert(doubleableHand);

    std::copy_n(doubleableHand, sizeof d_doubleableHand, d_doubleableHand);
}

// ACCESSORS

// Player Rules

bool Rules::playerMayDoubleOnTheseTwoCards(int handValue, bool isSoftCount) const
{
    assert(2 <= handValue);
    assert(handValue <= 20);
    assert(handValue >= 4 || isSoftCount);
    assert(handValue <= 11 || !isSoftCount);

    const int handIndex = isSoftCount ? State::soft(handValue) : State::hard(handValue);

    return playerMayDoubleOnAnyTwoCards() || d_doubleableHand[handIndex];
}

} // namespace bjgb

// FREE OPERATORS
bool bjgb::operator==(const Rules& lhs, const Rules& rhs)
{
    if (lhs.dealerStandsOnSoft17() != rhs.dealerStandsOnSoft17() || lhs.playerMaySurrender() != rhs.playerMaySurrender()
        || lhs.playerGetsOneCardOnlyToSplitAce() != rhs.playerGetsOneCardOnlyToSplitAce()
        || lhs.playerMayResplitAces() != rhs.playerMayResplitAces()
        || lhs.playerCanLoseDouble() != rhs.playerCanLoseDouble()
        || lhs.playerMayDoubleOnAnyTwoCards() != rhs.playerMayDoubleOnAnyTwoCards()
        || lhs.playerMaxNumHands() != rhs.playerMaxNumHands()
        || lhs.playerBlackjackPayout() != rhs.playerBlackjackPayout()) {
        return false; // RETURN
    }

    for (int i = 0; i <= State::k_NUM_STATES; ++i) {
        if (lhs.d_doubleableHand[i] != rhs.d_doubleableHand[i]) {
            return false; // RETURN
        }
    }

    return true;
}

std::ostream& bjgb::operator<<(std::ostream& stream, const Rules& rules)
{
    stream << "================ Blackjack Rules ================\n";

    stream << "Dealer must stand on soft 17:           " << rules.dealerStandsOnSoft17() << '\n';

    stream << "Player may surrender:                   " << rules.playerMaySurrender() << '\n';

    stream << "Player may resplit Aces:                " << rules.playerMayResplitAces() << '\n';

    stream << "Player gets 1 card only to a split Ace: " << rules.playerGetsOneCardOnlyToSplitAce() << '\n';

    stream << "Player may double on any 2 cards:       " << rules.playerMayDoubleOnAnyTwoCards() << '\n';

    if (!rules.playerMayDoubleOnAnyTwoCards()) {
        // TBD print out array of doubleable hands
    }

    stream << "Player can lose double:                 " << rules.playerCanLoseDouble() << '\n';

    stream << "Player max number of hands:             " << rules.playerMaxNumHands() << '\n';

    stream << "Player blackjack payout:                " << rules.playerBlackjackPayout() << '\n';

    stream << "=================================================\n";

    return stream;
}
