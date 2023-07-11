#include <prjvhc_vehicle.h>

Codethink::prjvhc::Vehicle::Vehicle()
{
}

Codethink::prjvhc::Vehicle::~Vehicle()
{
}

void Codethink::prjvhc::Vehicle::drive()
{
    drive_impl();
}

void Codethink::prjvhc::Vehicle::setActor(Codethink::prjact::Actor *actor)
{
    m_actor = actor;
}

void Codethink::prjvhc::Vehicle::drive_impl()
{
    int e = 0;
    for(int i = 0; i < 10; i++) {
        e = i;
    }
    e = e + 0;
    (void) e;
}

