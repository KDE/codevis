# Setup the plugin headers dependency
set(ENV{LVTPLG_HDRS} ${CMAKE_SOURCE_DIR}/lvtplg/)

add_subdirectory(code_coverage_plugin)
add_subdirectory(cycle_detection_plugin)
