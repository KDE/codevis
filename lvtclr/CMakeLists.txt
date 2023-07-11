AddTargetLibrary(
  LIBRARY_NAME
    lvtclr
  SOURCES
    ct_lvtclr_colormanagement.cpp
  QT_HEADERS
    ct_lvtclr_colormanagement.h
  LIBRARIES
    Qt5::Core
    lakospreferences
)

if (COMPILE_TESTS)
    add_executable(test_ct_lvtclr_colormanagement ct_lvtclr_colormanagement.t.cpp)
    target_link_libraries(test_ct_lvtclr_colormanagement
        Codethink::lvttst
        Codethink::lvtclr
        ${EXTRA_SYSTEM_LIBRARIES}
    )
    add_test(NAME test_ct_lvtclr_colormanagement COMMAND test_ct_lvtclr_colormanagement)
endif()
