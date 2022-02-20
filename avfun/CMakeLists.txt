set(PRO_NAME avfun)

## support c++17
set(CMAKE_CXX_STANDARD 17)

set(AVFUN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(${AVFUN_ROOT}/cmake/sdl.cmake)
include(${AVFUN_ROOT}/cmake/opencv.cmake)
include(${AVFUN_ROOT}/cmake/libyuv.cmake)
include(${AVFUN_ROOT}/cmake/ffmpeg.cmake)

####################### glad #######################
if (WIN32)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/win)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
elseif (ANDROID)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/android)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
elseif (APPLE)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/mac)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
else ()
    set(GLAD_SOURCE)
endif ()

####################### source #######################

set(AVFUN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CODEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/codec)
set(RENDER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render)

set(SOURCE_FILE
        ${AVFUN_DIR}/AVCommon.h
        ${AVFUN_DIR}/LogUtil.h
        ${AVFUN_DIR}/AVPlayer.h
        ${AVFUN_DIR}/AVPlayer.cpp

        ${CODEC_DIR}/AVFBuffer.h
        ${CODEC_DIR}/ffmpeg_config.h
        ${CODEC_DIR}/AVFReader.h
        ${CODEC_DIR}/AVFAudioReader.h
        ${CODEC_DIR}/AVFAudioReader.cpp
        ${CODEC_DIR}/AVFAudioResample.h
        ${CODEC_DIR}/AVFAudioResample.cpp
        ${CODEC_DIR}/AVVideoFrame.h
        ${CODEC_DIR}/AVVideoFrame.cpp
        ${CODEC_DIR}/AVFVideoReader.h
        ${CODEC_DIR}/AVFVideoReader.cpp


        ${RENDER_DIR}/GLProgram.h
        ${RENDER_DIR}/GLProgram.cpp
        ${RENDER_DIR}/GLTexture.h
        ${RENDER_DIR}/GLTexture.cpp
        )

list(APPEND SOURCE_FILE ${GLAD_SOURCE})

source_group_by_files(SOURCE_FILE)

add_library(${PRO_NAME} STATIC ${SOURCE_FILE})

target_link_libraries(${PRO_NAME} PUBLIC ${FFMPEG_LIBS} yuv ${OpenCV_LIBS} SDL2)
target_include_directories(${PRO_NAME} PUBLIC
        ${AVFUN_DIR} ${GLAD_DIR} ${libyuv_INCLUDE} ${OpenCV_INCLUDE_DIRS})

####################### example #######################
add_executable(${PRO_NAME}-example01 ${AVFUN_DIR}/example/example01.cpp)
target_link_libraries(${PRO_NAME}-example01 PRIVATE ${PRO_NAME})

add_executable(${PRO_NAME}-example02 ${AVFUN_DIR}/example/example02.cpp)
target_link_libraries(${PRO_NAME}-example02 PRIVATE ${PRO_NAME})

#add_executable(${PRO_NAME}-example03 ${AVFUN_DIR}/example/example03.cpp)
#target_link_libraries(${PRO_NAME}-example03 PRIVATE ${PRO_NAME})

add_executable(${PRO_NAME}-example04 ${AVFUN_DIR}/example/example04.cpp)
target_link_libraries(${PRO_NAME}-example04 PRIVATE ${PRO_NAME})

#file(COPY ${ffmpeg_DIR}/bin/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY ${OpenCV_DIR}/x64/vc15/bin/opencv_world451d.dll 
#${OpenCV_DIR}/x64/vc15/bin/opencv_world451.dll 
#${OpenCV_DIR}/x64/vc15/bin/opencv_videoio_ffmpeg451_64.dll 
#DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY ${sdl_DIR}/build/Debug/SDL2d.dll 
#${sdl_DIR}/build/Release/SDL2.dll 
#DESTINATION ${CMAKE_CURRENT_BINARY_DIR})