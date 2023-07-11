// bjgc_dealertable.cpp                                               -*-C++-*-
#include <bjgc_dealertable.h>

#include <iomanip>
#include <ostream>

// STATIC HELPER FUNCTIONS
static void printDealerColumnLines(std::ostream& stream)
{
    stream << "____ "; // card column

    for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
        stream << "_________ ";
    }

    stream << '\n';
}

namespace bjgc {

// -----------------
// class DealerTable
// -----------------

// MANIPULATORS
void DealerTable::clearRow(int hand)
{
    assert(0 <= hand);
    assert(hand < bjgb::State::k_NUM_STATES);

    for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
        prob(hand, i) = 0.0;
    }
}

void DealerTable::reset()
{
    for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
        for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
            prob(j, i) = -9999.99;
        }
    }
}

} // namespace bjgc

// FREE OPERATORS
std::ostream& bjgc::operator<<(std::ostream& stream, const DealerTable& table)
{
    const int k_COL_WID = 9; // change long underline to match

    //         =========_
    stream << "HAND "
              "   17     "
              "   18     "
              "   19     "
              "   20     "
              "   21     "
              "   BJ     "
              "  OVER    "
              "\n";

    printDealerColumnLines(stream);

    for (int j = bjgb::State::k_NUM_STATES - 1; j >= 0; --j) {
        stream << ' ' << std::setw(2) << bjgb::State::stateId2String(j) << ": ";

        for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
            stream << std::setw(k_COL_WID) << table.prob(j, i) << ' ';
        }
        stream << '\n';

        if (bjgb::State::e_HZR == j || bjgb::State::hard(2) == j || bjgb::State::pair(1) == j) {
            printDealerColumnLines(stream);
        }
    }

    printDealerColumnLines(stream);

    return stream;
}
