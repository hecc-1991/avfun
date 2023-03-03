cmake_minimum_required(VERSION 3.18.1)

project("avfuncore")

set(AVFUN_ROOT /Users/hecc/develop/project/avfun/)
add_subdirectory(${AVFUN_ROOT}/avfun avfun)

add_library(
        avfuncore
        SHARED
        avfun_Context.cpp
        avfun_Player.cpp
        GLContext.h
        GLContext.cpp
        EglThread.h
        EglThread.cpp
)

target_link_libraries(
        avfuncore
#        avfun
        android
        log
        EGL
        GLESv2
        )