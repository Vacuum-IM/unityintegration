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
include_directories("${DBUSMENUQT_INCLUDE_DIRS}")

pkg_check_modules(MESSAGINGMENU messaging-menu)

set(MESSAGING_MENU OFF CACHE BOOL "Unity Messaging Menu and Launcher API support. Only Ubuntu 14.04 and newer.")

if(${MESSAGING_MENU} AND MESSAGINGMENU_FOUND)
	message(STATUS "Building with Unity Messaging Menu and Launcher API support")
	add_definitions(-DMESSAGING_MENU)
	set(SOURCES ${SOURCES} thirdparty/messaging-menu-qt/messaging-menu-qt.cpp)
	set(HEADERS ${HEADERS} thirdparty/messaging-menu-qt/messaging-menu-qt.hpp)
	set(MOC_HEADERS ${MOC_HEADERS} thirdparty/messaging-menu-qt/messaging-menu-qt.hpp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	set(ADD_LIBS ${ADD_LIBS} ${MESSAGINGMENU_LIBRARIES})
	include_directories("${MESSAGINGMENU_INCLUDE_DIRS}")
elseif(${MESSAING_MENU})
	message(FATAL_ERROR "Building without unity Messaging Menu API support. Check messaging-menu library dependency.")
	return()
else()
	message(STATUS "Building without unity Messaging Menu API support. Enable: -DMESSAGING_MENU=ON")
endif()

include_directories("${CMAKE_SOURCE_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

#install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
