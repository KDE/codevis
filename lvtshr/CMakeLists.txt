# Model Libraries
# Those libraries defines a standard way for the interface
# To query data and present in list, tables and trees.

AddTargetLibrary(
  LIBRARY_NAME
    lvtshr
  SOURCES
    ct_lvtshr_functional.cpp
    ct_lvtshr_fuzzyutil.cpp
    ct_lvtshr_graphenums.cpp
    ct_lvtshr_graphstorage.cpp
    ct_lvtshr_iterator.cpp
    ct_lvtshr_loaderinfo.cpp
    ct_lvtshr_stringhelpers.cpp
    ct_lvtshr_uniqueid.cpp

    ct_lvtshr_functional.h
    ct_lvtshr_fuzzyutil.h
    ct_lvtshr_graphenums.h
    ct_lvtshr_graphstorage.h
    ct_lvtshr_iterator.h
    ct_lvtshr_loaderinfo.h
    ct_lvtshr_stringhelpers.h
    ct_lvtshr_uniqueid.h
  LIBRARIES
    Qt${QT_MAJOR_VERSION}::Core
)

if (COMPILE_TESTS)
    function(ADD_LVTSHR_TESTCASE TC_NAME)
        MESSAGE(STATUS "Adding test ${TC_NAME}")

        add_executable(test_${TC_NAME}
            ${TC_NAME}.t.cpp
        )

        target_link_libraries(test_${TC_NAME}
            Codethink::lvtshr
            Codethink::lvttst
        )

        add_test(NAME test_${TC_NAME} COMMAND test_${TC_NAME})
    endfunction()

    add_lvtshr_testcase(ct_lvtshr_fuzzyutil)
    add_lvtshr_testcase(ct_lvtshr_stringhelpers)
    add_lvtshr_testcase(ct_lvtshr_iterator)
endif()
