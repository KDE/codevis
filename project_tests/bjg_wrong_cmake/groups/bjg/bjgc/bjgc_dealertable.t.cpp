// bjgc_dealertable.t.cpp                                             -*-C++-*-
#include <bjgc_dealertable.h>

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

        // default constructor
        {
            const bjgc::DealerTable X;

            for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
                for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
                    ASSERT(X.prob(j, i) == -9999.99);
                }
            }
        }

        // 'clearRow', 'reset'
        {
            bjgc::DealerTable mX;
            const bjgc::DealerTable& X = mX;

            for (int k = 0; k < bjgb::State::k_NUM_STATES; ++k) {
                mX.clearRow(k);

                for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
                    for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
                        ASSERT(X.prob(j, i) == (j <= k ? 0.0 : -9999.99));
                    }
                }
            }

            mX.reset();

            for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
                for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
                    ASSERT(X.prob(j, i) == -9999.99);
                }
            }
        }

        // 'prob' manipulator and accessor
        {
            bjgc::DealerTable mX;
            const bjgc::DealerTable& X = mX;

            for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
                for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
                    mX.prob(j, i) = j * 2 + i * 3;
                }
            }

            for (int j = 0; j < bjgb::State::k_NUM_STATES; ++j) {
                for (int i = 0; i < bjgb::DealerCount::k_NUM_FINAL_COUNTS; ++i) {
                    ASSERT(X.prob(j, i) == j * 2 + i * 3);
                }
            }
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
