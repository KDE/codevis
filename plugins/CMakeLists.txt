# Setup the plugin headers dependency
set(ENV{LVTPLG_HDRS} ${CMAKE_BINARY_DIR}/plugin-headers/)

add_subdirectory(code_coverage_plugin)
add_subdirectory(cycle_detection_plugin)
add_subdirectory(lakosian_test_rules_plugin)
