cmake_minimum_required(VERSION 3.5)

project(DSV VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL Gui Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL Gui Core)

add_subdirectory(src)
include_directories(src)

set(PROJECT_SOURCES
    src/main.cpp
    ${_BASE_HEADS}
    ${_DRAW_HEADS}
    ${_IO_HEADS}
    ${_PARSER_HEADS}
    ${_UI_HEADS}
    deps/EarCut/EarCut.hpp

    ${_BASE_SOURCES}
    ${_DRAW_SOURCES}
    ${_IO_SOURCES}
    ${_UI_SOURCES} 

    ${_UI_FILES}

    src/resource/Icons.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DSV
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(DSV
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(DSV PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(DSV PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS DSV
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DSV)
endif()
