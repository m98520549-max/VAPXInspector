cmake_minimum_required(VERSION 3.21)
project(VAPXInspector VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia)

qt_add_executable(VAPXInspector
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/VapParser.cpp
    src/VapParser.h
    src/PreviewWidget.cpp
    src/PreviewWidget.h
)

target_link_libraries(VAPXInspector PRIVATE Qt6::Widgets Qt6::Multimedia)
target_include_directories(VAPXInspector PRIVATE src)
if(MSVC)
    target_compile_options(VAPXInspector PRIVATE /W4 /utf-8)
endif()
