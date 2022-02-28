cmake_minimum_required(VERSION 3.18.2)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_C_STANDARD 98)

project(ffplay)

set(AVFUN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

set(THIRD_PARTY_DIR ${AVFUN_ROOT}/third_party)
set(ffmpeg_DIR ${THIRD_PARTY_DIR}/ffmpeg_build)
set(ffmpeg_INCLUDE ${THIRD_PARTY_DIR}/ffmpeg_build/include)
include_directories(${THIRD_PARTY_DIR}/ffmpeg)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -liconv -lm -llzma -lz -framework AudioToolbox -pthread -framework VideoToolbox -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -lm -lbz2 -lz -Wl,-framework,CoreFoundation -Wl,-framework,Security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -framework Foundation -framework AudioToolbox -framework CoreAudio -lm -framework AVFoundation -framework CoreVideo -framework CoreMedia -framework CoreGraphics -L/usr/local/Cellar/libxcb/1.14_1/lib -lxcb -L/usr/local/Cellar/libxcb/1.14_1/lib -lxcb-shm -L/usr/local/Cellar/libxcb/1.14_1/lib -lxcb-shape -L/usr/local/Cellar/libxcb/1.14_1/lib -lxcb-xfixes -framework AudioToolbox -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -framework OpenGL -framework OpenGL -pthread -lm -framework Metal -framework VideoToolbox -framework CoreImage -framework AppKit -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices")

add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libavcodec.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

add_library(avdevice STATIC IMPORTED)
set_target_properties(avdevice PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libavdevice.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"

        )

add_library(avfilter STATIC IMPORTED)
set_target_properties(avfilter PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libavfilter.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libavformat.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libavutil.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libswresample.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

add_library(swscale STATIC IMPORTED)
set_target_properties(swscale PROPERTIES
        IMPORTED_LOCATION "${ffmpeg_DIR}/lib/libswscale.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_INCLUDE}"
        )

set(FFMPEG_LIBS avcodec avdevice avfilter avformat avutil swresample swscale)

set(CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR}/SDL2_build)
find_package(SDL2)
if(SDL2_FOUND)
else(SDL2_FOUND)
    message(FATAL_ERROR ”SDL2 library not found”)
endif(SDL2_FOUND)

add_executable(ffplay
        ffplay.c
        cmdutils.h
        cmdutils.c)

target_link_libraries(ffplay PUBLIC ${FFMPEG_LIBS} ${SDL2_LIBRARIES})

target_include_directories(ffplay PUBLIC ${SDL2_INCLUDE_DIRS})