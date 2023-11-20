cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0135 NEW)

set(ENV{http_proxy} http://localhost:7890)
set(ENV{https_proxy} http://localhost:7890)

include(FetchContent)

FetchContent_Declare(
    qpdf
    GIT_REPOSITORY https://github.com/qpdf/qpdf
    GIT_TAG v11.6.3
)
# windows环境独立配置zlib和jpeg库,非windows环境要自行配置qpdf依赖的zlib和jpeg库
if (WIN32)
    # 先行下载qpdf
    message(STATUS "Downloading qpdf ......")
    FetchContent_GetProperties(qpdf)
    if(NOT qpdf_POPULATED)
        FetchContent_Populate(qpdf)
    endif()

    set(QPDF_EXTERNAL_LIBS_URL "https://github.com/qpdf/external-libs/releases/download/release-2023-10-07/qpdf-external-libs-bin.zip")
    set(QPDF_EXTERNAL_LIBS_SHA256 "9ff36e54aeabdf91984b72e30cc52018b25ce63e2ff57573b7302eae29403340")
    set(QPDF_EXTERNAL_LIBS_DEST ${qpdf_SOURCE_DIR})
    set(QPDF_EXTERNAL_LIBS_DIR_NAME "external-libs")
    set(QPDF_EXTERNAL_LIBS_ZIP_NAME "qpdf-external-bin.zip")
    set(QPDF_EXTERNAL_LIBS_FILENAME "${qpdf_SOURCE_DIR}/${QPDF_EXTERNAL_LIBS_FILENAME}")

    unset(QEL_DIR )
    unset(QEL_DIR CACHE)
    unset(QEL_ZIP)
    unset(QEL_ZIP CACHE)

    find_file(QEL_DIR
                NAME ${QPDF_EXTERNAL_LIBS_DIR_NAME}
                PATHS ${QPDF_EXTERNAL_LIBS_DEST}
                NO_DEFAULT_PATH)
    find_file(QEL_ZIP
                NAME ${QPDF_EXTERNAL_LIBS_ZIP_NAME}
                PATHS ${QPDF_EXTERNAL_LIBS_DEST}
                NO_DEFAULT_PATH)
    message(STATUS "Checking qpdf external-libs ${QPDF_EXTERNAL_LIBS_DEST}/${QPDF_EXTERNAL_LIBS_ZIP_NAME} ......")

    if(NOT QEL_DIR)
    # 不存在external-libs 目录，开始处理
        if(QEL_ZIP)
            file(SHA256 ${QEL_ZIP} sha)
            if(NOT "${sha}" EQUAL "${QPDF_EXTERNAL_LIBS_SHA256}")
                message(STATUS "${QEL_ZIP} existed but sha256 doesn't match. Removing and redownload.")
                file(REMOVE ${QEL_ZIP})
                unset(QEL_ZIP)
                unset(QEL_ZIP CACHE)
            endif()
        endif()
        if(NOT QEL_ZIP)
            message(STATUS "Downloading ${QPDF_EXTERNAL_LIBS_ZIP_NAME} to ${QPDF_EXTERNAL_LIBS_DEST} ......")
            file(DOWNLOAD ${QPDF_EXTERNAL_LIBS_URL}
                    ${QPDF_EXTERNAL_LIBS_DEST}/${QPDF_EXTERNAL_LIBS_ZIP_NAME}
                    # PROXY ${proxy}
                    TIMEOUT 60
                    STATUS ERR
                    SHOW_PROGRESS)
            
            if(ERR EQUAL 0)
                set(QEL_ZIP "${QPDF_EXTERNAL_LIBS_DEST}/${QPDF_EXTERNAL_LIBS_ZIP_NAME}")
            else()
                MESSAGE(STATUS "Download failed, error: ${ERR}")
                MESSAGE(FATAL_ERROR 
                    "You can try downloading ${QPDF_EXTERNAL_LIBS_URL} and place it to ${QPDF_EXTERNAL_LIBS_DEST} manually"
                    " using curl/wget or a similar tool")
            endif()
        endif()
        message(STATUS "cd ${QPDF_EXTERNAL_LIBS_DEST}; tar xfz ${QPDF_EXTERNAL_LIBS_ZIP_NAME}")
        #下载文件基本无问题，进入解压路径开始解压压缩包
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xfz "${QEL_ZIP}"
                WORKING_DIRECTORY "${QPDF_EXTERNAL_LIBS_DEST}"
                RESULT_VARIABLE tar_result
        )
        #判断解压是否成功
        if(tar_result MATCHES 0)
            set(QPDF_EXTERNAL_LIBS_FOUND 1 CACHE INTERNAL "")
        else()
            MESSAGE(STATUS "Failed to extract files.\n"
                "   Please try downloading and extracting yourself.\n"
                "   The url is: ${QPDF_EXTERNAL_LIBS_URL}")
        endif()
        
    endif()

    add_subdirectory(${qpdf_SOURCE_DIR} ${qpdf_BINARY_DIR})
else()
    FetchContent_MakeAvailable(qpdf)
endif()
