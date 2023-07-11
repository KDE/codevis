// bjgc_playertable.cpp                                               -*-C++-*-
#include <bjgc_playertable.h>

#include <iomanip>
#include <ostream>

// STATIC HELPER FUNCTIONS
static void printPlayerColumnLines(std::ostream& stream)
{
    stream << "____ "; // card column

    for (int i = 1; i <= bjgc::PlayerTable::k_NUM_CARD_VALUES; ++i) {
        stream << "______ ";
    }

    stream << '\n';
}

namespace bjgc {

// -----------------
// class PlayerTable
// -----------------

// MANIPULATORS
void PlayerTable::clearRow(int hand)
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);

    for (int i = 1; i <= k_NUM_CARD_VALUES; ++i) {
        exp(hand, i) = 0.0;
    }
}

void PlayerTable::reset()
{
    for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
        for (int i = 1; i <= k_NUM_CARD_VALUES; ++i) {
            exp(j, i) = -9e97;
        }
    }
}

void PlayerTable::setRow(int hand, bjgb::Types::Double value)
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);

    for (int i = 1; i <= k_NUM_CARD_VALUES; ++i) {
        exp(hand, i) = value;
    }
}

} // namespace bjgc

// FREE OPERATORS
std::ostream& bjgc::operator<<(std::ostream& stream, const PlayerTable& table)
{
    const int k_COL_WID = 6; // change long underline to match

    const int oldPrecision = stream.precision();

    stream << std::setprecision(4);

    //         ======_
    stream << "CARD "
              "  ACE  "
              "   2   "
              "   3   "
              "   4   "
              "   5   "
              "   6   "
              "   7   "
              "   8   "
              "   9   "
              "  TEN  "
              "\n";

    printPlayerColumnLines(stream);

    for (int j = bjgb::State::k_NUM_STATES - 1; j >= 0; --j) {
        stream << ' ' << std::setw(2) << bjgb::State::stateId2String(j) << ": ";

        for (int i = 1; i <= PlayerTable::k_NUM_CARD_VALUES; ++i) {
            stream << std::setw(k_COL_WID) << 100 * table.exp(j, i) << ' ';
        }

        stream << '\n';

        if (bjgb::State::e_HZR == j || bjgb::State::hard(2) == j || bjgb::State::pair(1) == j) {
            printPlayerColumnLines(stream);
        }
    }

    printPlayerColumnLines(stream);

    stream.precision(oldPrecision);

    return stream;
}
