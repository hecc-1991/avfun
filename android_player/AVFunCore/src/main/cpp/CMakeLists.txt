cmake_minimum_required(VERSION 3.18.1)

project("avfuncore")

set(AVFUN_ROOT /Users/hecc/develop/project/avfun/)
add_subdirectory(${AVFUN_ROOT}/avfun avfun)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

####################### glad #######################
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render/glad/android)
set(GLAD_SOURCE ${GLAD_DIR}/glad/glad.h ${GLAD_DIR}/KHR/khrplatform.h ${GLAD_DIR}/glad.c)
include_directories(${GLAD_DIR})

set(RENDER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/render)
include_directories(${RENDER_DIR})

set(SOURCE_FILE
        AVCommon.h
        Utils.h
        com_avfuncore_AVFContext.cpp
        com_avfuncore_AVFPlayer.cpp
        VideoFrameBuffer.cpp
        VideoFrameBuffer.h
        AVPlayer.cpp
        AVPlayer.h
        GLContext.h
        GLContext.cpp
        EglThread.h
        EglThread.cpp
        ${RENDER_DIR}/GLProgram.h
        ${RENDER_DIR}/GLProgram.cpp
        ${RENDER_DIR}/GLTexture.h
        ${RENDER_DIR}/GLTexture.cpp
        ${GLAD_SOURCE}
        )

add_library(
        avfuncore
        SHARED

        ${SOURCE_FILE}
)



target_link_libraries(
        avfuncore
#        avfun
        android
        log
        EGL
        GLESv2
        mediandk
        OpenMAXAL
        )