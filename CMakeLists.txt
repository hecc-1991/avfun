cmake_minimum_required(VERSION 3.10)

project(avfun-master)

####################### function #######################

## 建立文件目录结构

add_subdirectory(avfun)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Android")
add_subdirectory(win_player)
endif()
