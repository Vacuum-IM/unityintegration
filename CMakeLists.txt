cmake_minimum_required(VERSION 2.8)

project(unityintegration-plugin)
set(PLUGIN_NAME unityintegration) # defaults to CMAKE_PROJECT_NAME, if not set

set(SOURCES unityintegration.cpp) # list of all files to be compiled
set(HEADERS unityintegration.h) # list of all header files
set(MOC_HEADERS unityintegration.h) # files to be moc-ified
#set(UIS pluginname.ui)

# Also you can use next commands instead of manual file enumeration:
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")
file(GLOB UIS "**.ui")

#set(ADD_LIBS "dbusmenu-qt")
#include_directories("${CMAKE_INSTALL_PREFIX}/include/dbusmenu-qt/")

FIND_PACKAGE(PkgConfig)
pkg_check_modules(DBUSMENUQT dbusmenu-qt>=0.8.1)

LINK_LIBRARIES (${DBUSMENUQT_LIBRARIES})
include_directories (${DBUSMENUQT_INCLUDE_DIRS})

if (NOT (UNIX AND NOT APPLE))
message(FATAL_ERROR "Supported only on *nix")
endif (NOT (UNIX AND NOT APPLE)) 

# By default, only QtNetwork and QtXml modules are linked, if you need other
# modules - set corresponding variables for FindQt4 CMake module:
# http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:FindQt4
set(QT_USE_QTDBUS ON)
#set(QT_USE_QTWEBKIT ON)

# Build system assumes that *.ts files are named translations/${lang}/${pluginname}.ts
set(LOCALIZED_LANGS "ru;pl;de;uk")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/translations.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")


