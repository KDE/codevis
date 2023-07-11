// bjgb_shoe.cpp                                                      -*-C++-*-
#include <bjgb_shoe.h>

#include <iomanip>
#include <ostream>

namespace bjgb {

// ----------
// class Shoe
// ----------

// PRIVATE MANIPULATORS
void Shoe::updateCache()
{
    d_totalCards = 0;

    for (int i = 1; i <= 10; ++i) {
        d_totalCards += d_rankCounts[i];
    }

    const Types::Double denom = d_totalCards;

    for (int i = 1; i <= 10; ++i) {
        d_prob[i] = d_rankCounts[i] / denom;
    }
}

// CREATORS
Shoe::Shoe(int numDecks)
{
    assert(1 <= numDecks);
    assert(numDecks <= 8);

    d_rankCounts[0] = 0; // unused but must set

    for (int i = 1; i <= 10; ++i) {
        d_rankCounts[i] = 4 * numDecks;
    }

    d_rankCounts[10] *= 4; // four times as many T cards

    updateCache();
}

Shoe::Shoe(const int *rankCounts)
{
    assert(rankCounts);

    setNumCardsOfEachRank(rankCounts);
}

// MANIPULATORS
void Shoe::setNumCardsOfEachRank(const int *rankCounts)
{
    assert(rankCounts);

    d_rankCounts[0] = 0; // unused but must set

    for (int i = 0; i < 10; ++i) {
        assert(0 <= rankCounts[i]);
        assert(rankCounts[i] <= (9 == i ? 128 : 32));

        d_rankCounts[i + 1] = rankCounts[i];
    }

    updateCache();
}

} // namespace bjgb

// FREE OPERATORS
bool bjgb::operator==(const Shoe& lhs, const Shoe& rhs)
{
    if (lhs.numCardsTotal() != rhs.numCardsTotal()) {
        return false; // RETURN
    }

    using namespace RankLiterals;

    for (Rank r = A_R; r < Rank::end(); ++r) {
        if (lhs.numCards(r) != rhs.numCards(r)) {
            return false; // RETURN
        }
    }

    return true;
}

std::ostream& bjgb::operator<<(std::ostream& stream, const Shoe& shoe)
{
    stream << "Shoe:\n";

    for (int i = 1; i <= 10; ++i) {
        stream << std::setw(8) << i;
    }

    using namespace RankLiterals;

    for (Rank r = A_R; r < Rank::end(); ++r) {
        stream << std::setw(8) << shoe.numCards(r);
    }

    for (int i = 1; i <= 10; ++i) {
        stream << std::setw(8) << shoe.prob(i) * 100;
    }

    return stream << "\n";
}
