cmake_minimum_required(VERSION 3.17.5)

set(PRO_NAME avfun)

## support c++17
set(CMAKE_CXX_STANDARD 17)

set(AVFUN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

include(${AVFUN_ROOT}/cmake/platform.cmake)
include(${AVFUN_ROOT}/cmake/utils.cmake)

if(AVF_WIN32 OR AVF_LINUX OR AVF_MACOS)
    include(${AVFUN_ROOT}/cmake/sdl.cmake)
    include(${AVFUN_ROOT}/cmake/opencv.cmake)
    include(${AVFUN_ROOT}/cmake/libyuv.cmake)
    include(${AVFUN_ROOT}/cmake/ffmpeg.cmake)
elseif(AVF_ANDROID)
    include(${AVFUN_ROOT}/cmake/opencv.cmake)
    include(${AVFUN_ROOT}/cmake/libyuv.cmake)
    include(${AVFUN_ROOT}/cmake/ffmpeg.cmake)
else()

endif()

####################### glad #######################
if (AVF_WIN32)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/win)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
elseif (AVF_ANDROID)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/android)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
elseif (AVF_MACOS)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/mac)
    set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
elseif (AVF_LINUX)
    set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/linux)
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
        ${AVFUN_DIR}/Utils.h
        ${AVFUN_DIR}/LogUtil.h
        ${AVFUN_DIR}/AVPlayer.h

        ${CODEC_DIR}/AVFBuffer.h
        ${CODEC_DIR}/ffmpeg_config.h
        ${CODEC_DIR}/AVFReader.h
        ${CODEC_DIR}/AVFAudioReader.h
        ${CODEC_DIR}/AVFAudioResample.h
        ${CODEC_DIR}/AVFAudioResample.cpp
        ${CODEC_DIR}/AVVideoFrame.h
        ${CODEC_DIR}/AVVideoFrame.cpp
        ${CODEC_DIR}/AVFVideoReader.h

        ${RENDER_DIR}/GLProgram.h
        ${RENDER_DIR}/GLProgram.cpp
        ${RENDER_DIR}/GLTexture.h
        ${RENDER_DIR}/GLTexture.cpp
        )

if(AVF_WIN32 OR AVF_LINUX OR AVF_MACOS)

    list(APPEND SOURCE_FILE
            ${AVFUN_DIR}/AVPlayer.cpp
            ${CODEC_DIR}/AVFAudioReader.cpp
            ${CODEC_DIR}/AVFVideoReader.cpp

            )

elseif(AVF_ANDROID)

    list(APPEND SOURCE_FILE
            ${AVFUN_DIR}/AVPlayerAndroid.cpp
            ${CODEC_DIR}/android/AVFAudioReader.cpp
            ${CODEC_DIR}/android/AVFVideoReader.cpp
            )

else()

endif()

list(APPEND SOURCE_FILE ${GLAD_SOURCE})

source_group_by_files(SOURCE_FILE)

add_library(${PRO_NAME} STATIC ${SOURCE_FILE})

target_link_libraries(${PRO_NAME} PUBLIC ${FFMPEG_LIBS} yuv ${OpenCV_LIBS} ${SDL2_LIBRARIES})
target_include_directories(${PRO_NAME} PUBLIC
        ${AVFUN_DIR} ${CODEC_DIR} ${GLAD_DIR} ${libyuv_INCLUDE} ${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

####################### example #######################
if(AVF_WIN32 OR AVF_LINUX OR AVF_MACOS)

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

else()

endif()