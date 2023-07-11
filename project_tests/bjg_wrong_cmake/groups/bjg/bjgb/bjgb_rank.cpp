// bjgb_rank.cpp                                                      -*-C++-*-
#include <bjgb_rank.h>

#include <ostream>

// ----------
// class Rank
// ----------

// FREE OPERATORS
std::ostream& bjgb::operator<<(std::ostream& stream, const Rank& rank)
{
    char rep;

    switch (rank.value()) {
    case 1: {
        rep = 'A';
    } break;
    case 2: {
        rep = '2';
    } break;
    case 3: {
        rep = '3';
    } break;
    case 4: {
        rep = '4';
    } break;
    case 5: {
        rep = '5';
    } break;
    case 6: {
        rep = '6';
    } break;
    case 7: {
        rep = '7';
    } break;
    case 8: {
        rep = '8';
    } break;
    case 9: {
        rep = '9';
    } break;
    case 10: {
        rep = 'T';
    } break;
    default: {
        assert("Invalid 'Rank' state for streaming." && 0);

        rep = '?';
    } break;
    }

    return stream << rep;
}
