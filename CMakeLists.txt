cmake_minimum_required(VERSION 3.20.0)
project(DSV VERSION 0.5.1)

include(CTest)
enable_testing()

if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/arch:AVX2")
    add_compile_options("/GL")
    add_compile_options("/MP")
    add_link_options("/LTCG")
    add_link_options("/NODEFAULTLIB:LIBCMT.lib")
elseif (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options("-m64")
    add_compile_options("-mavx2")
    add_compile_options("-funroll-loops")
    add_compile_options("-ffunction-sections")
    add_compile_options("-fdata-sections")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets OpenGL Gui Core)
find_package(GSL REQUIRED)

include_directories(libs)
add_subdirectory(libs)

add_subdirectory(src)
include_directories(src)
file(GLOB _UI_HEADS src/ui/*hpp)

qt_add_executable(DSV
    MANUAL_FINALIZATION
    src/main.cpp
    ${_UI_HEADS}
    src/draw/Canvas.hpp

    ${_DRAW_SOURCES}
    ${_BASE_SOURCES}
    ${_UI_SOURCES}
    ${_IO_SOURCES}
    ${_LIBRARY_SOURCES}
    res/icon.rc
    res/Icons.qrc
)

target_link_libraries(DSV PRIVATE Qt6::Widgets Qt6::OpenGLWidgets Qt6::OpenGL Qt6::Gui Qt6::Core GSL::gsl)

set_target_properties(DSV PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(DSV)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

install(
  TARGETS DSV
  # RUNTIME_DEPENDENCIES DIRECTORIES /usr/lib 
  DESTINATION bin
)

install(
  FILES DSV.desktop
  DESTINATION share/applications/
)

install(
  FILES res/resource/DSV.ico
  DESTINATION share/icons/hicolor/128x128/apps
)
