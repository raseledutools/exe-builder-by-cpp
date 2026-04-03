QT       += core gui widgets

CONFIG   += c++17

# উইন্ডোজের কনসোল হাইড করা
win32 {
    CONFIG -= console
    # Windows এর সিস্টেম ফাংশন লিঙ্কার (খুবই জরুরি)
    LIBS   += -luser32 -lshell32 -ladvapi32 -ldwmapi -lgdi32 -lOle32
}

# প্রোজেক্টের নাম
TARGET   = "RasFocus+AdultBlocker"
TEMPLATE = app

SOURCES  += main.cpp

# আইকন ফাইল থাকলে আনকমেন্ট করো
# RC_ICONS = icon.ico
