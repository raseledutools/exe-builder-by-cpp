QT       += core gui widgets
CONFIG   += c++17

# MSVC-তে কনসোল (CMD) হাইড করার সঠিক নিয়ম
win32: CONFIG += release windows

TARGET = RasFocus_Pro
TEMPLATE = app

SOURCES += main.cpp \
           adblocker.cpp

# আইকন ফাইল
RC_FILE = app.rc

# Windows API Libraries (Linker Errors Fix)
win32: LIBS += -lwininet -ldwmapi -lole32 -luuid -luser32 -ladvapi32
