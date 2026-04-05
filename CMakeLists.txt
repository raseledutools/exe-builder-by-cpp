cmake_minimum_required(VERSION 3.5)

project(FocusPro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Auto Generate tools (খুব ইম্পর্ট্যান্ট, সিগন্যাল-স্লট কাজ করার জন্য)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt Libraries
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Executable output (এখানে app.rc যুক্ত করা হয়েছে আইকনের জন্য)
add_executable(FocusPro WIN32 main.cpp app.rc)

# Link Libraries (Qt + ALL required Win32 APIs)
target_link_libraries(FocusPro PRIVATE 
    Qt5::Widgets
    user32 
    gdi32 
    comctl32 
    advapi32 
    shell32 
    ws2_32 
    dwmapi 
    wininet 
    ole32 
    uuid
)
