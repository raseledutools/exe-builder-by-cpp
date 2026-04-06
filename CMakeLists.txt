cmake_minimum_required(VERSION 3.5)
project(RasFocus_Pro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt5 খুঁজবে
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# WIN32 ফ্ল্যাগটি CMD উইন্ডো হাইড করবে
add_executable(RasFocus_Pro WIN32 
    RasFocus_Pro.cpp 
    adblocker.cpp 
    app.rc
)

target_link_libraries(RasFocus_Pro PRIVATE 
    Qt5::Widgets 
    wininet 
    dwmapi 
    ole32 
    uuid
)
