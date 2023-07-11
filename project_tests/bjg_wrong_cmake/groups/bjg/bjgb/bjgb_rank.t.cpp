// bjgb_rank.t.cpp                                                    -*-C++-*-
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

        using namespace bjgb::RankLiterals;

        // derived from Usage in header file
        {
            const char *EXP = "A 2 3 4 5 6 7 8 9 T";

            std::ostringstream os;

            for (bjgb::Rank r = A_R; r != bjgb::Rank::end(); ++r) {
                if (r != A_R) {
                    os << ' ';
                }
                os << r;
            }

            ASSERT(EXP == os.str());
        }

        // exercise full compliment of literals
        {
            ASSERT(1 == (1_R).value());
            ASSERT(2 == (2_R).value());
            ASSERT(3 == (3_R).value());
            ASSERT(4 == (4_R).value());
            ASSERT(5 == (5_R).value());
            ASSERT(6 == (6_R).value());
            ASSERT(7 == (7_R).value());
            ASSERT(8 == (8_R).value());
            ASSERT(9 == (9_R).value());
            ASSERT(10 == (10_R).value());

            ASSERT(A_R == 1_R);
            ASSERT(T_R == 10_R);

            ASSERT(!(1_R == 2_R));
            ASSERT(1_R != 2_R);
            ASSERT(1_R < 2_R);

            ASSERT(!(2_R == 1_R));
            ASSERT(2_R != 1_R);
            ASSERT(!(2_R < 1_R));

            ASSERT(10_R < bjgb::Rank::end());
            ASSERT(++(10_R) == bjgb::Rank::end());
        }

        // copy
        {
            bjgb::Rank r7 = 7_R;
            bjgb::Rank r7c(r7);

            ASSERT(r7 == r7c);

            constexpr bjgb::Rank r8 = 8_R;

            r7 = r8;

            ASSERT(r7 == r8);
            ASSERT(r7 != r7c);
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
