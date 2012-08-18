#Plugin file name
TARGET = unityintegration
include(config.inc)

#Project Configuration
TEMPLATE		 = lib
CONFIG			+= plugin
QT				 = core gui dbus xml

LIBS			+= -l$${TARGET_UTILS}
LIBS			+= -L$${VACUUM_LIB_PATH}
DEPENDPATH		+= $${VACUUM_SRC_PATH}
INCLUDEPATH		+= $${VACUUM_SRC_PATH}

#Only fedora has pkgconfig patch for dbusmenu-qt package, not ubuntu
LIBS			+= -ldbusmenu-qt
INCLUDEPATH		+= $$INSTALL_PREFIX/include/dbusmenu-qt

#Install
include(install.inc)

#Translation
include(translations.inc)

#Code
include(unityintegration.pri)

