# this library acts a Project Manager
# for the project.

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

AddTargetLibrary(
  LIBRARY_NAME
    lvtprj
  SOURCES
    ct_lvtprj_projectfile.cpp
  HEADERS
    ct_lvtprj_projectfile.h

  LIBRARIES
    ${SYSTEM_EXTRA_LIBRARIES}
    Codethink::lvtmdb
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::Archive
    Codethink::lvtshr
)

if (COMPILE_TESTS)
    add_executable(test_lvtprj_project ct_lvtprj_projectfile.t.cpp)
    target_link_libraries(test_lvtprj_project Codethink::lvtprj lvttst)
    add_test(NAME test_lvtprj COMMAND test_lvtprj_project)
endif()
