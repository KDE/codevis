// bjgb_state.cpp                                                     -*-C++-*-
#include <bjgb_state.h>

namespace bjgb {

// ------------
// struct State
// ------------

static_assert(11 == State::e_SBJ);
static_assert(12 == State::e_HZR);
static_assert(13 == State::e_S_A);
static_assert(22 == State::e_H_T);

static_assert(State::e_H_T == State::e_S_A + 9);
static_assert(State::e_PTT == State::e_PAA + 9);

// CLASS METHODS
const char *State::stateId2String(int stateId)
{
    static const char names[k_NUM_STATES][3] = {// soft count
                                                "S1",
                                                "S2",
                                                "S3",
                                                "S4",
                                                "S5",
                                                "S6",
                                                "S7",
                                                "S8",
                                                "S9",
                                                "ST",
                                                "SA",

                                                // blackjack
                                                "BJ",

                                                // zero cards
                                                "ZR",

                                                // one card
                                                "_A",
                                                "_2",
                                                "_3",
                                                "_4",
                                                "_5",
                                                "_6",
                                                "_7",
                                                "_8",
                                                "_9",
                                                "_T",

                                                // hard count
                                                "02",
                                                "03",
                                                "04",
                                                "05",
                                                "06",
                                                "07",
                                                "08",
                                                "09",
                                                "10",
                                                "11",
                                                "12",
                                                "13",
                                                "14",
                                                "15",
                                                "16",
                                                "17",
                                                "18",
                                                "19",
                                                "20",
                                                "21",

                                                // over
                                                "OV",

                                                // two identical cards
                                                "AA",
                                                "22",
                                                "33",
                                                "44",
                                                "55",
                                                "66",
                                                "77",
                                                "88",
                                                "99",
                                                "TT"};

    assert(0 <= stateId);
    assert(stateId < k_NUM_STATES);

    return names[stateId];
}

} // namespace bjgb
