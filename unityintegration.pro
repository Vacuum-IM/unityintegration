include(qmake/debug.inc)
include(qmake/config.inc)

contains(DEFINES, MESSAGING_MENU): {
  CONFIG             += link_pkgconfig
  PKGCONFIG          += messaging-menu
  QMAKE_CXXFLAGS     += -std=c++0x
}

#Project configuration
TARGET              = unityintegration
QT                  = core gui dbus xml
include(unityintegration.pri)

#Default progect configuration
include(qmake/plugin.inc)

#Translation
TRANS_SOURCE_ROOT   = .
TRANS_BUILD_ROOT = $${OUT_PWD}
include(translations/languages.inc)

#Only fedora has pkgconfig patch for dbusmenu-qt package, not ubuntu
LIBS               += -ldbusmenu-qt
INCLUDEPATH        += $$INSTALL_PREFIX/include/dbusmenu-qt
