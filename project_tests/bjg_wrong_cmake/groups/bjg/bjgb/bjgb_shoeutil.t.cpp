// bjgb_shoeutil.t.cpp                                                -*-C++-*-
#include <bjgb_shoeutil.h>

#include <bjgb_rank.h>

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

        // 'isUnused'
        {
            const bjgb::Shoe X;

            ASSERT(true == bjgb::ShoeUtil::isUnused(X));
            ASSERT(0 == bjgb::ShoeUtil::tenRichness(X));

            bjgb::Shoe mY(2);
            const bjgb::Shoe& Y = mY;

            ASSERT(true == bjgb::ShoeUtil::isUnused(Y));
            ASSERT(0 == bjgb::ShoeUtil::tenRichness(Y));

            using namespace bjgb::RankLiterals;

            mY.setNumCardsOfRank(1_R, 9);

            ASSERT(false == bjgb::ShoeUtil::isUnused(Y));
            ASSERT(0 > bjgb::ShoeUtil::tenRichness(Y));

            mY.setNumCardsOfRank(2_R, 7);

            ASSERT(false == bjgb::ShoeUtil::isUnused(Y));
            ASSERT(0 == bjgb::ShoeUtil::tenRichness(Y));

            mY.setNumCardsOfRank(3_R, 7);

            ASSERT(false == bjgb::ShoeUtil::isUnused(Y));
            ASSERT(0 < bjgb::ShoeUtil::tenRichness(Y));
        }

        // 'setTenRichness', 'tenRichness'
        {
            const bjgb::Shoe X;
            ASSERT(0 == bjgb::ShoeUtil::tenRichness(X));

            bjgb::Shoe m2;
            bjgb::ShoeUtil::setTenRichness(&m2, -2);
            bjgb::Shoe m1;
            bjgb::ShoeUtil::setTenRichness(&m1, -1);
            bjgb::Shoe p0;
            bjgb::ShoeUtil::setTenRichness(&p0, +0);
            bjgb::Shoe p1;
            bjgb::ShoeUtil::setTenRichness(&p1, +1);
            bjgb::Shoe p2;
            bjgb::ShoeUtil::setTenRichness(&p2, +2);

            ASSERT(-2 == bjgb::ShoeUtil::tenRichness(m2));
            ASSERT(-1 == bjgb::ShoeUtil::tenRichness(m1));
            ASSERT(0 == bjgb::ShoeUtil::tenRichness(p0));
            ASSERT(1 == bjgb::ShoeUtil::tenRichness(p1));
            ASSERT(2 == bjgb::ShoeUtil::tenRichness(p2));
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
