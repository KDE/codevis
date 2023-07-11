// bjgb_shoe.t.cpp                                                    -*-C++-*-
#include <bjgb_shoe.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

// ============================================================================
//                        STANDARD ASSERT TEST FUNCTION
// ----------------------------------------------------------------------------

namespace {

int testStatus = 0;

void aSsErT(bool condition, const char *message, int line)
{
    if (condition) {
        std::cout << "Error " __FILE__ "(" << line << "): " << message << "    (failed)" << std::endl;

        if (0 <= testStatus && testStatus <= 100) {
            ++testStatus;
        }
    }
}

} // namespace

// ============================================================================
//                         STANDARD TEST DRIVER MACROS
// ----------------------------------------------------------------------------

#define ASSERT(X) aSsErT(!(X), #X, __LINE__);

// ============================================================================
//                              MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    const int test = argc > 1 ? std::atoi(argv[1]) : 0;
    const bool verbose = argc > 2;
    const bool veryVerbose = argc > 3;
    const bool veryVeryVerbose = argc > 4;
    const bool veryVeryVeryVerbose = argc > 5;

    std::cout << "TEST " << __FILE__ << " CASE " << test << std::endl;

    switch (test) {
    case 0:
    case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST
        //
        // Concerns:
        //: 1 TBD
        //
        // Plan:
        //: 1 TBD
        //
        // Testing:
        //   BREATHING TEST
        // --------------------------------------------------------------------

        if (verbose)
            std::cout << std::endl << "BREATHING TEST" << std::endl << "==============" << std::endl;

        using namespace bjgb::RankLiterals;

        // default construct
        {
            bjgb::Shoe mX;
            const bjgb::Shoe& X = mX;

            ASSERT(312 == X.numCardsTotal()); // 6 decks

            bjgb::Rank r = A_R;

            while (r < T_R) {
                ASSERT(24 == X.numCards(r)); // 24 each of A, 2, ..., 9

                ++r;
            }

            ASSERT(96 == X.numCards(r)); // 4 times as many tens
        }

        // 'setNumCardsOfRank'
        {
            bjgb::Shoe mX(4);
            const bjgb::Shoe& X = mX;

            ASSERT(208 == X.numCardsTotal()); // 4 decks

            bjgb::Rank r = A_R;

            while (r < T_R) {
                ASSERT(16 == X.numCards(r)); // 16 each of A, 2, ..., 9

                ++r;
            }

            ASSERT(64 == X.numCards(r)); // 4 times as many tens

            const bjgb::Shoe Y; // 6 decks

            ASSERT(X != Y);

            r = A_R;

            while (r < T_R) {
                mX.setNumCardsOfRank(r, 24);

                ++r;
            }
            ASSERT(X != Y);

            mX.setNumCardsOfRank(r, 96);
            ASSERT(X == Y);
        }

        // 'setNumCardsOfEachRank'
        {
            const int counts[] = {2, 4, 6, 8, 10, 20, 24, 26, 28, 30};

            const bjgb::Shoe X(counts);

            int ix = 0;

            for (bjgb::Rank r = A_R; r != bjgb::Rank::end(); ++r) {
                ASSERT(counts[ix] == X.numCards(r));
                ++ix;
            }

            bjgb::Shoe mY(8);
            const bjgb::Shoe& Y = mY;

            ASSERT(X != Y);

            mY.setNumCardsOfEachRank(counts);

            ASSERT(X == Y);
        }

    } break;
    default: {
        std::cerr << "WARNING: CASE `" << test << "' NOT FOUND." << std::endl;
        testStatus = -1;
    }
    }

    if (testStatus > 0) {
        std::cerr << "Error, non-zero test status = " << testStatus << "." << std::endl;
    }

    return testStatus;
}
