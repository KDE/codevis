This is a small test project that creates two packages,
one that defines a vehicle, and another that defines an actor.
an actor can have a vehicle, andr drive it around.
the vehicle has tires, and gas.

It's important that the projects have zero includes from the system
because those can change between compilers, and we want something
that can be stable for testing purposes, so there will not be
any output from the application.

We are not interested in correct code - the implementations
can be empty, but we need to take inconsideration the `uses in implementation`
features, so not completely empty.
