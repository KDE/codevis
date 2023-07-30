# this library acts a Project Manager
# for the project.

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

AddTargetLibrary(
  LIBRARY_NAME
    lvtprj
  SOURCES
    ct_lvtprj_projectfile.cpp
  QT_HEADERS
    ct_lvtprj_projectfile.h

  LIBRARIES
    ${SYSTEM_EXTRA_LIBRARIES}
    Codethink::lvtmdb
    quazip
    Qt5::Core
    Codethink::lvtshr
)

add_executable(codevis_create_prj_from_db
    ct_lvtprj_create_prj_from_db.m.cpp
)
target_link_libraries(codevis_create_prj_from_db
    Codethink::lvtprj
    Codethink::lvtldr
)

if (COMPILE_TESTS)
    add_executable(test_lvtprj_project ct_lvtprj_projectfile.t.cpp)
    target_link_libraries(test_lvtprj_project Codethink::lvtprj lvttst)
    add_test(NAME test_lvtprj COMMAND test_lvtprj_project)
endif()
