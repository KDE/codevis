set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_executable(my_cmake_test_qsettings test.cpp)
target_link_libraries(my_cmake_test_qsettings Qt5::Core)
include(ConfigurationCompiler)
compile_configuration(my_cmake_test_qsettings GENERATOR qsettings demo_qsettings.conf)

find_package(KF5Config)
add_executable(my_cmake_test_kconfig test.cpp)
target_link_libraries(my_cmake_test_kconfig Qt5::Core KF5::ConfigCore)
include(ConfigurationCompiler)
compile_configuration(my_cmake_test_kconfig GENERATOR kconfig demo_kconfig.conf)

add_executable(my_cmake_test_qobject test.cpp)
target_link_libraries(my_cmake_test_qobject Qt5::Core)
include(ConfigurationCompiler)
compile_configuration(my_cmake_test_qobject GENERATOR qobject demo_qobject.conf)

enable_testing()
add_definitions(-g)

include_directories(${CMAKE_SOURCE_DIR})

#add_executable(
#    generator_tests
#    unittests.cpp)

#target_link_libraries(generator_tests confgen_lib Qt5::Core)

add_custom_target(check
    COMMAND
        ${CMAKE_CTEST_COMMAND}
    DEPENDS
        pre-check
        unittests
)

FILE(GLOB KCONFIG_TESTS *.kconfig*)
FILE(GLOB QOBJECT_TESTS *.qobject*)
FILE(GLOB QSETTINGS_TESTS *.qsettings*)

add_custom_target(pre-check  COMMAND cp ${KCONFIG_TESTS} ${QOBJECT_TESTS} ${QSETTINGS_TESTS} ${CMAKE_BINARY_DIR}/test)

# add_test(GenTests generator_tests)
