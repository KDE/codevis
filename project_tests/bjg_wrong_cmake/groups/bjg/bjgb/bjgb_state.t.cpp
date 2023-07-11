// bjgb_state.t.cpp                                                   -*-C++-*-
#include <bjgb_state.h>

#include <cstdlib>
#include <cstring>
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

typedef bjgb::State::Enum Enum;
typedef bjgb::State Util;

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

            ASSERT(value++ == Util::e_S01);
            ASSERT(value++ == Util::e_S02);
            ASSERT(value++ == Util::e_S03);
            ASSERT(value++ == Util::e_S04);
            ASSERT(value++ == Util::e_S05);
            ASSERT(value++ == Util::e_S06);
            ASSERT(value++ == Util::e_S07);
            ASSERT(value++ == Util::e_S08);
            ASSERT(value++ == Util::e_S09);
            ASSERT(value++ == Util::e_S10);
            ASSERT(value++ == Util::e_S11);
            ASSERT(value++ == Util::e_SBJ);
            ASSERT(value++ == Util::e_HZR);
            ASSERT(value++ == Util::e_S_A);
            ASSERT(value++ == Util::e_H_2);
            ASSERT(value++ == Util::e_H_3);
            ASSERT(value++ == Util::e_H_4);
            ASSERT(value++ == Util::e_H_5);
            ASSERT(value++ == Util::e_H_6);
            ASSERT(value++ == Util::e_H_7);
            ASSERT(value++ == Util::e_H_8);
            ASSERT(value++ == Util::e_H_9);
            ASSERT(value++ == Util::e_H_T);
            ASSERT(value++ == Util::e_H02);
            ASSERT(value++ == Util::e_H03);
            ASSERT(value++ == Util::e_H04);
            ASSERT(value++ == Util::e_H05);
            ASSERT(value++ == Util::e_H06);
            ASSERT(value++ == Util::e_H07);
            ASSERT(value++ == Util::e_H08);
            ASSERT(value++ == Util::e_H09);
            ASSERT(value++ == Util::e_H10);
            ASSERT(value++ == Util::e_H11);
            ASSERT(value++ == Util::e_H12);
            ASSERT(value++ == Util::e_H13);
            ASSERT(value++ == Util::e_H14);
            ASSERT(value++ == Util::e_H15);
            ASSERT(value++ == Util::e_H16);
            ASSERT(value++ == Util::e_H17);
            ASSERT(value++ == Util::e_H18);
            ASSERT(value++ == Util::e_H19);
            ASSERT(value++ == Util::e_H20);
            ASSERT(value++ == Util::e_H21);
            ASSERT(value++ == Util::e_HOV);
            ASSERT(value++ == Util::e_PAA);
            ASSERT(value++ == Util::e_P22);
            ASSERT(value++ == Util::e_P33);
            ASSERT(value++ == Util::e_P44);
            ASSERT(value++ == Util::e_P55);
            ASSERT(value++ == Util::e_P66);
            ASSERT(value++ == Util::e_P77);
            ASSERT(value++ == Util::e_P88);
            ASSERT(value++ == Util::e_P99);
            ASSERT(value++ == Util::e_PTT);

            ASSERT(value == Util::k_NUM_STATES);
        }

        // sanity check limits of 'hard', 'soft', 'pair', 'unus'
        {
            ASSERT(Util::e_H02 == Util::hard(2));
            ASSERT(Util::e_H21 == Util::hard(21));

            ASSERT(Util::e_S01 == Util::soft(1));
            ASSERT(Util::e_S11 == Util::soft(11));

            ASSERT(Util::e_PAA == Util::pair(1));
            ASSERT(Util::e_PTT == Util::pair(10));

            ASSERT(Util::e_S_A == Util::unus(1));
            ASSERT(Util::e_H_T == Util::unus(10));
        }

        // values returned by 'stateId2String' are unique
        {
            for (int i = 0; i < Util::k_NUM_STATES; ++i) {
                for (int j = 0; j < Util::k_NUM_STATES; ++j) {
                    const bool isSame = 0 == std::strcmp(Util::stateId2String(i), Util::stateId2String(j));
                    ASSERT((i == j) == isSame);
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
