cmake_minimum_required(VERSION 3.5)
project(RasFocus_Pro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ফোরস স্ট্যাটিক লিংকিং (যাতে কোনো DLL না খোঁজে, শুধু .a ফাইল নেয়)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

enable_language(RC)

# Qt5 খুঁজবে
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# WIN32 ফ্ল্যাগটি CMD উইন্ডো হাইড করবে
add_executable(RasFocus_Pro WIN32 
    main.cpp 
    adblocker.cpp 
    app.rc
)

set_target_properties(RasFocus_Pro PROPERTIES WIN32_EXECUTABLE ON)

target_link_libraries(RasFocus_Pro PRIVATE 
    Qt5::Widgets 
    Qt5::Core
    Qt5::Gui
    wininet 
    dwmapi 
    ole32 
    uuid
)
