// bjgb_dealercount.t.cpp                                             -*-C++-*-
#include <bjgb_dealercount.h>

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
//                        GLOBAL TYPEDEFS FOR TESTING
// ----------------------------------------------------------------------------

typedef bjgb::DealerCount::Enum Enum;
typedef bjgb::DealerCount Util;

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

        // starting from 0, the enumerators are sequential in value
        {
            int value = 0;

            ASSERT(value++ == Util::e_C17);
            ASSERT(value++ == Util::e_C18);
            ASSERT(value++ == Util::e_C19);
            ASSERT(value++ == Util::e_C20);
            ASSERT(value++ == Util::e_C21);
            ASSERT(value++ == Util::e_CBJ);
            ASSERT(value++ == Util::e_COV);

            ASSERT(value == Util::k_NUM_FINAL_COUNTS);
        }

        // sanity check limits of 'fini'
        {
            ASSERT(Util::e_C17 == Util::fini(17));
            ASSERT(Util::e_C21 == Util::fini(21));
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
