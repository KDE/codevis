#ifndef PRJVEHICLE_H
#define PRJVEHICLE_H

#include <prjvhc_wheel.h>

namespace Codethink { namespace prjact { class Actor; }}

namespace Codethink {
namespace prjvhc {

class Vehicle {
public:
    Vehicle();
    ~Vehicle();

    void drive(); // noop.
    void setActor(prjact::Actor *actor);

    class InnerVehicle {
        // this should just create an inner vehicle inside of vehicle.
    };

private:
    void drive_impl();

    enum Wheels {
        E_FRONT_LEFT,
        E_FRONT_RIGHT,
        E_BACK_LEFT,
        E_BACK_RIGHT,
        COUNT
    };

    Wheel m_wheels[Wheels::COUNT];
    prjact::Actor *m_actor;
};

} // end namespace prjvhc
} // end namespace codethink

#endif
