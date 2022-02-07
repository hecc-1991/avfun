set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

set(ffmpeg_DIR ${THIRD_PARTY_DIR}/ffmpeg_build)
set(ffmpeg_INCLUDE ${THIRD_PARTY_DIR}/ffmpeg_build/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -liconv -lm -llzma -lz -framework AudioToolbox -pthread -framework VideoToolbox -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -lm -lbz2 -lz -Wl,-framework,CoreFoundation -Wl,-framework,Security")

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
