cmake_minimum_required(VERSION 3.20.0)
project(DSV VERSION 0.4.0)

include(CTest)
enable_testing()

set(USE_QPDF ON CACHE BOOL "使用从pdf导入功能" FORCE)
message(STATUS "Using QPDF ${USE_QPDF}")

configure_file(
    ${PROJECT_SOURCE_DIR}/inc/config.h.in
    ${PROJECT_SOURCE_DIR}/inc/config.h
)

if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("/arch:AVX2")
    add_compile_options("/bigobj")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
message(STATUS "Using Qt in ${Qt6_DIR}")

if(USE_QPDF)
    add_definitions(-DPOINTERHOLDER_TRANSITION=0)
    ## 编译qpdf
    # add_subdirectory(lib/qpdf)
    # set (EXTRA_LIBS ${EXTRA_LIBS} libqpdf)
    ## 设置qpdf路径 example: SET(qpdf_DIR D:/qpdf-11.6.1-msvc64/lib/cmake/qpdf)
    # SET(qpdf_DIR /set/your/qpdf/path)
    SET(qpdf_DIR D:/qpdf-11.6.1-msvc64/lib/cmake/qpdf)
    find_package(qpdf REQUIRED)
    if(qpdf_FOUND)
        message(STATUS "Using QPDF in ${qpdf_DIR}")
        set (EXTRA_LIBS ${EXTRA_LIBS} qpdf::libqpdf_static)
    else()
        message(FATAL_ERROR "QPDF library NOT FOUND")
    endif()
endif()

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
    ${_SIM_SOURCES}
    src/resource/icon.rc
)

target_link_libraries(DSV PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core ${EXTRA_LIBS})

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