#Plugin file name
TARGET = unityintegration
include(config.inc)

#Project Configuration
TEMPLATE = lib
CONFIG += plugin
QT = core gui dbus
LIBS += -l$${TARGET_UTILS}
LIBS += -L$${VACUUM_LIB_PATH}
DEPENDPATH += $${VACUUM_SRC_PATH}
INCLUDEPATH += $${VACUUM_SRC_PATH}
LIBS		    += -ldbusmenu-qt
INCLUDEPATH	    += $$INSTALL_PREFIX/include/dbusmenu-qt

#Install
include(install.inc)

#Translation
include(translations.inc)

#Code
include(unityintegration.pri)

