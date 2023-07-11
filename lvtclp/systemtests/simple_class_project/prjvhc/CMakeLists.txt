add_library(prjvhc
    SHARED
    prjvhc_vehicle.cpp
    prjvhc_wheel.cpp
)

include_directories(.)

target_include_directories(prjvhc
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
