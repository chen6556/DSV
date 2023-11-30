cmake_minimum_required(VERSION 3.20.0)
project(DSV VERSION 0.4.0)

include(CTest)
enable_testing()

if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("/arch:AVX2")
    add_compile_options("/bigobj")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Gui Core)
find_package(qpdf)

add_subdirectory(src)
include_directories(inc)

qt_add_executable(DSV
    MANUAL_FINALIZATION
    src/main.cpp
    inc/ui/LayersManager.hpp
    inc/ui/MainWindow.hpp
    inc/draw/Canvas.hpp

    ${_DRAW_SOURCES}
    ${_BASE_SOURCES}
    ${_UI_SOURCES}
    ${_IO_SOURCES}
    src/resource/icon.rc
)

target_link_libraries(DSV PRIVATE Qt6::Widgets Qt6::OpenGLWidgets Qt6::Gui Qt6::Core qpdf::libqpdf)

set_target_properties(DSV PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    # WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(DSV)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)