cmake_minimum_required(VERSION 2.8)

project(unityintegration-plugin)
set(PLUGIN_NAME unityintegration)

set(SOURCES unityintegration.cpp)
set(HEADERS unityintegration.h)
set(MOC_HEADERS unityintegration.h)

if (NOT (UNIX AND NOT APPLE))
  message(FATAL_ERROR "Supported only on *nix")
endif (NOT (UNIX AND NOT APPLE)) 

FIND_PACKAGE(PkgConfig)
pkg_check_modules(DBUSMENUQT dbusmenu-qt>=0.8.1)

if(NOT DBUSMENUQT_FOUND)
  message(FATAL_ERROR "Cannot build unity integration plugin, check dependencies: dbusmenu-qt>=0.8.1")
  return()
endif()

set(QT_USE_QTDBUS ON)
set(QT_USE_QTWEBKIT OFF)

set(ADD_LIBS ${DBUSMENUQT_LIBRARIES})
include_directories (${DBUSMENUQT_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

#install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
