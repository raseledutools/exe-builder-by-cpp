QT       += core gui widgets
CONFIG   += c++17

# কনসোল (CMD) হাইড করার জন্য
win32: CONFIG += release
win32: QMAKE_LFLAGS += -mwindows

TARGET = RasFocus_Pro
TEMPLATE = app

SOURCES += main.cpp \
           adblocker.cpp

# যদি আইকনের জন্য app.rc ফাইল থাকে
RC_FILE = app.rc

# Windows API Libraries
win32: LIBS += -lwininet -ldwmapi -lole32 -luuid
