cmake_minimum_required(VERSION 3.5)

project(FocusPro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt Libraries
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Executable output
add_executable(FocusPro WIN32 main.cpp)

# Link Libraries (Qt + Win32 API)
target_link_libraries(FocusPro PRIVATE Qt5::Widgets)
target_link_libraries(FocusPro PRIVATE user32 shell32 advapi32)
