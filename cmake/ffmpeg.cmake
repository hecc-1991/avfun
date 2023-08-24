set(AVFUN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(THIRD_PARTY_DIR ${AVFUN_ROOT}/third_party)


include(${AVFUN_ROOT}/cmake/platform.cmake)

if(AVF_LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lm -latomic")
elseif(AVF_MACOS)
    set(ffmpeg_DIR ${THIRD_PARTY_DIR}/ffmpeg/mac)
    set(ffmpeg_INCLUDE ${THIRD_PARTY_DIR}/ffmpeg/mac/include)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -liconv -lm -llzma -lz -framework AudioToolbox -pthread -framework VideoToolbox -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -lm -lbz2 -lz -Wl,-framework,CoreFoundation -Wl,-framework,Security")
else()

endif()

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

set(FFMPEG_LIBS avformat avcodec avdevice avfilter avutil swresample swscale)
