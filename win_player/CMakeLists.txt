set(PRO_NAME win_player)

## support c++17
set(CMAKE_CXX_STANDARD 17)

#set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

set(AVFUN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
include(${AVFUN_ROOT}/cmake/ffmpeg.cmake)

####################### sdl #######################
#set(sdl_DIR ${THIRD_PARTY_DIR}/SDL2-2.0.14)

####################### source #######################

set(PLAYER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

	
set(IMGUI_SOURCE 
${PLAYER_DIR}/imgui/imconfig.h
${PLAYER_DIR}/imgui/imgui.cpp
${PLAYER_DIR}/imgui/imgui.h
${PLAYER_DIR}/imgui/imgui_demo.cpp
${PLAYER_DIR}/imgui/imgui_draw.cpp
${PLAYER_DIR}/imgui/imgui_internal.h
${PLAYER_DIR}/imgui/imgui_tables.cpp
${PLAYER_DIR}/imgui/imgui_widgets.cpp
${PLAYER_DIR}/imgui/imstb_rectpack.h
${PLAYER_DIR}/imgui/imstb_textedit.h
${PLAYER_DIR}/imgui/imstb_truetype.h

${PLAYER_DIR}/imgui/imgui_impl_sdl.h
${PLAYER_DIR}/imgui/imgui_impl_sdl.cpp
${PLAYER_DIR}/imgui/imgui_impl_opengl3.h
${PLAYER_DIR}/imgui/imgui_impl_opengl3.cpp

)

set(SOURCE_FILE 
${PLAYER_DIR}/main.cpp
${PLAYER_DIR}/App.h
${PLAYER_DIR}/App.cpp
${PLAYER_DIR}/MainWindow.h
${PLAYER_DIR}/MainWindow.cpp
${PLAYER_DIR}/ControlView.h
${PLAYER_DIR}/ControlView.cpp
${PLAYER_DIR}/SurfaceView.h
${PLAYER_DIR}/SurfaceView.cpp
)

list(APPEND SOURCE_FILE ${IMGUI_SOURCE})

source_group_by_files(SOURCE_FILE)

add_executable(${PRO_NAME} ${SOURCE_FILE})

target_link_libraries(${PRO_NAME} PRIVATE avfun)
#target_include_directories(${PRO_NAME} PUBLIC )

#if (WIN32)
#    file(COPY ${sdl_DIR}/build/Debug/SDL2d.dll
#            ${sdl_DIR}/build/Release/SDL2.dll
#            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#endif ()

