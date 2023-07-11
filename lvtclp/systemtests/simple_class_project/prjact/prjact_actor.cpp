#include <prjact_actor.h>

#include <prjvhc_vehicle.h>

using namespace Codethink::prjact;

Actor::Actor()
: m_vehicle_p(nullptr) {

}

Actor::~Actor() {

}

void Actor::setVehicle(Codethink::prjvhc::Vehicle *vehicle)
{
    m_vehicle_p = vehicle;
    if (vehicle) {
        vehicle->drive();
    }
}
