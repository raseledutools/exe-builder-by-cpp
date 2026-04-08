cmake_minimum_required(VERSION 3.5)
project(RasFocus_Pro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Resource file (.rc) কম্পাইল করার জন্য RC ল্যাঙ্গুয়েজ এনাবল করতে হবে (MinGW এর জন্য জরুরি)
enable_language(RC)

# Qt5 খুঁজবে
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# WIN32 ফ্ল্যাগটি CMD উইন্ডো হাইড করবে
add_executable(RasFocus_Pro WIN32 
    main.cpp 
    adblocker.cpp 
    app.rc
)

# স্ট্যাটিক বিল্ডের জন্য প্রয়োজনীয় ফ্ল্যাগ (যাতে সব .dll ফাইলের কোড .exe এর ভেতর ঢুকে যায়)
set_target_properties(RasFocus_Pro PROPERTIES WIN32_EXECUTABLE ON)
target_link_options(RasFocus_Pro PRIVATE "-static" "-static-libgcc" "-static-libstdc++")

target_link_libraries(RasFocus_Pro PRIVATE 
    Qt5::Widgets 
    Qt5::Core
    Qt5::Gui
    wininet 
    dwmapi 
    ole32 
    uuid
)
