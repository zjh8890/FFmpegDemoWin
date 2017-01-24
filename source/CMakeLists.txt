cmake_minimum_required(VERSION 2.8)
project(ffmpegDemoWin)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DEBUG")
set (DLL_TO_DBG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set (DLL_TO_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
set (DEBUG_DLLS ${PROJECT_SOURCE_DIR}/../ffmpeg/dll/)
set (RELEASE_DLLS ${PROJECT_SOURCE_DIR}/../ffmpeg/dll/)

aux_source_directory(./ SRC_LIST)


file(COPY ${RELEASE_DLLS}avcodec-56.dll DESTINATION ${DLL_TO_RELEASE})
file(COPY ${RELEASE_DLLS}avformat-56.dll DESTINATION ${DLL_TO_RELEASE})
file(COPY ${RELEASE_DLLS}avutil-54.dll DESTINATION ${DLL_TO_RELEASE})

file(COPY ${DEBUG_DLLS}avcodec-56.dll DESTINATION ${DLL_TO_DBG})
file(COPY ${DEBUG_DLLS}avformat-56.dll DESTINATION ${DLL_TO_DBG})
file(COPY ${DEBUG_DLLS}avutil-54.dll DESTINATION ${DLL_TO_DBG})

include_directories(${PROJECT_SOURCE_DIR}/../ffmpeg/include)
link_directories(${PROJECT_SOURCE_DIR}/../ffmpeg/lib)

add_executable(ffmpegDemoWin ${SRC_LIST})
set_property(TARGET ffmpegDemoWin APPEND PROPERTY LINK_FLAGS /DEBUG)
target_link_libraries(ffmpegDemoWin avcodec.lib avutil.lib avformat.lib)

