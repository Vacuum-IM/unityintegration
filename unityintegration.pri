HEADERS = unityintegration.h
SOURCES = unityintegration.cpp

contains(DEFINES, MESSAGING_MENU): {
HEADERS += thirdparty/messaging-menu-qt/messaging-menu-qt.hpp
SOURCES += thirdparty/messaging-menu-qt/messaging-menu-qt.cpp
}
