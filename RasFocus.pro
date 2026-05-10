QT       += core gui widgets
CONFIG   += c++17

win32: CONFIG += release windows

TARGET   = RasFocus_Pro
TEMPLATE = app

SOURCES += main.cpp

win32: LIBS += -lwininet -ldwmapi -lole32 -luuid -luser32 -ladvapi32
