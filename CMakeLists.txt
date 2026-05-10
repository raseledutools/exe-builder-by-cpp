cmake_minimum_required(VERSION 3.16)

project(PDFEditorProCore VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 বা Qt5 খুঁজবে
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(PDFEditorProCore
    main.cpp
)

target_link_libraries(PDFEditorProCore PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
