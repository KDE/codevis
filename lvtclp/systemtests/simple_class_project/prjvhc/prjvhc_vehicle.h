#ifndef PRJVEHICLE_H
#define PRJVEHICLE_H

#include <prjvhc_wheel.h>

namespace Codethink {
namespace prjvhc {

class Vehicle {
public:
    Vehicle();
    ~Vehicle();

    void drive(); // noop.

private:
    void drive_impl();

    enum Wheels: short {
        E_FRONT_LEFT,
        E_FRONT_RIGHT,
        E_BACK_LEFT,
        E_BACK_RIGHT,
        COUNT
    };

    Wheel m_wheels[Wheels::COUNT];
};

} // end namespace prjvhc
} // end namespace codethink

#endif
