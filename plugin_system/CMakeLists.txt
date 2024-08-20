include_directories(${CMAKE_CURRENT_SOURCE_DIR})

AddTargetLibrary(
  LIBRARY_NAME
    PluginSystemHeaders
  SOURCES
    ICodevisPlugin.cpp
    IGraphicsLayoutPlugin.cpp
 HEADERS
    ICodevisPlugin.h
    IGraphicsLayoutPlugin.h

  LIBRARIES
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::CoreAddons
)

AddTargetLibrary(
  LIBRARY_NAME
    PluginSystem
  SOURCES
    PluginManagerV2.cpp
  HEADERS
    PluginManagerV2.h

  LIBRARIES
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::CoreAddons
)

