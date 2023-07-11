#ifndef PRJACTOR_H
#define PRJACTOR_H

namespace Codethink::prjvhc { class Vehicle; }

namespace Codethink {
namespace prjact {

class Actor {
public:
    Actor();
    ~Actor();

    void setVehicle(prjvhc::Vehicle *vehicle);
    prjvhc::Vehicle *vehicle() const;

private:
    // The usage of a raw pointer is because we can't use std::shared_ptr here,
    // as we can't depend on system includes *at all*.
    Codethink::prjvhc::Vehicle *m_vehicle_p;
};

} // end namespace prjact
} // end namespace codethink

#endif
