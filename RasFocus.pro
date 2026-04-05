QT += core gui widgets network
CONFIG += c++17
CONFIG -= app_bundle
TARGET = RasFocus_Pro

# Windows Libraries for Win32 API logic
LIBS += -luser32 -lgdi32 -lcomctl32 -ladvapi32 -lshell32 -lws2_32 -ldwmapi -lwininet -lole32 -luuid

SOURCES += \
    main.cpp \
    adblocker.cpp

RC_FILE = app.rc
