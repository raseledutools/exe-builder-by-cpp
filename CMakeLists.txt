cmake_minimum_required(VERSION 3.16)
project(PDFEditorProCore VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 খুঁজবে (Widgets + PrintSupport দুটোই লাগবে)
find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

# Poppler-Qt6 খুঁজবে
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-qt6)

add_executable(PDFEditorProCore
    main.cpp
)

target_include_directories(PDFEditorProCore PRIVATE
    ${POPPLER_INCLUDE_DIRS}
)

target_link_libraries(PDFEditorProCore PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    ${POPPLER_LIBRARIES}
)
