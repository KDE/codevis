#include <prjact_actor.h>
#include <prjvhc_vehicle.h>

int main() {
    using namespace Codethink::prjact;
    using namespace Codethink::prjvhc;

    Actor actor;
    auto *vehicle = new Vehicle();

    actor.setVehicle(vehicle);

}
