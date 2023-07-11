#include <prjvhc_wheel.h>

using namespace Codethink::prjvhc;

Wheel::Wheel()
{
    setBrand("Michelyear");
}

Wheel::~Wheel()
{

}

void Wheel::setBrand(char const *brandName)
{
    m_brandName = brandName;
}
