// bjgb_shoeutil.cpp                                                  -*-C++-*-
#include <bjgb_shoeutil.h>

#include <bjgb_rank.h>

#include <cassert>
#include <iostream> // TBD TEMPORARY

namespace bjgb {

// ---------------
// struct ShoeUtil
// ---------------

// CLASS METHODS
bool ShoeUtil::isUnused(const Shoe& shoe)
{
    using namespace RankLiterals;

    const int count = shoe.numCards(A_R);

    assert(count >= 4);

    if (0 != count % 4) {
        std::cout << "[unused: RETURN count = " << count << "]\n";
        return false;
    }

    for (Rank r = 2_R; r < T_R; ++r) {
        if (count != shoe.numCards(r)) {
            std::cout << "[unused: RETURN r = " << r << ", " << shoe.numCards(r) << "]\n";
            return false;
        }
    }

    if (count * 4 != shoe.numCards(T_R)) {
        std::cout << "[unused: RETURN 10: shoe.numCards(T_R)] = " << shoe.numCards(T_R) << "]\n";
        return false;
    }

    return true;
}

void ShoeUtil::setTenRichness(Shoe *shoe, int n)
{
    assert(shoe);
    assert(n <= 9);

    using namespace RankLiterals;

    for (Rank r = A_R; r < T_R; ++r) {
        shoe->setNumCardsOfRank(r, 9 - n);
    }

    shoe->setNumCardsOfRank(T_R, 36); // 9 * 4
}

Types::Double ShoeUtil::tenRichness(const Shoe& shoe)
{
    using namespace RankLiterals;

    int u = 0; // u == number of cards under 'T_R'

    for (Rank r = A_R; r < T_R; ++r) {
        u += shoe.numCards(r);
    }

    const int t = shoe.numCards(T_R); // t == number of 'T_R' cards

    const int c = u + t; // c == total number of cards

    assert(c == shoe.numCardsTotal());

    //  t      4
    //  - = ------  =>  (13 - n)t = 4c  =>  13t - nt = 4c  =>  13t - 4c = nt
    //  c   13 - n
    //              =>  13t/t - 4c/t = n  =>  n = 13 - 4c/t
    //

    return 13 - 4.0 * c / t;
}

} // namespace bjgb
