add_library(prjact
    SHARED
    prjact_actor.cpp
)

target_link_libraries(prjact
    prjvhc
)

include_directories(.)

target_include_directories(prjact
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
