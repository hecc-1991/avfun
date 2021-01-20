set(PRO_NAME codec)

## support c++17
set(CMAKE_CXX_STANDARD 17)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

####################### sdl #######################
set(sdl_DIR ${THIRD_PARTY_DIR}/SDL2-2.0.14)
set(sdl_INCLUDE ${THIRD_PARTY_DIR}/SDL2-2.0.14/include)
add_subdirectory(${sdl_DIR} ${sdl_DIR}/build)

####################### opencv #######################
set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/build)
find_package(OpenCV REQUIRED)

if(OpenCV_FOUND)
	message(STATUS "------------OpenCV ${OpenCV_VERSION} fonud-----------")	
	message(STATUS "-- OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
	message(STATUS "-- OpenCV_LIBS = ${OpenCV_LIBS}")
else(OpenCV_FOUND)
	message(FATAL_ERROR "OpenCV not found")
endif(OpenCV_FOUND)



####################### libyuv #######################
set(libyuv_DIR ${THIRD_PARTY_DIR}/libyuv)
set(libyuv_INCLUDE ${THIRD_PARTY_DIR}/libyuv/include)
add_subdirectory(${libyuv_DIR} ${libyuv_DIR}/build EXCLUDE_FROM_ALL)


####################### ffmpeg #######################

set(ffmpeg_DIR ${THIRD_PARTY_DIR}/ffmpeg)
set(ffmpeg_INCLUDE ${THIRD_PARTY_DIR}/ffmpeg/include)

#include_directories(${ffmpeg_DIR}/include)

add_library(avcodec SHARED IMPORTED)
set_property(TARGET avcodec PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/avcodec.lib"
			)
			
add_library(avdevice SHARED IMPORTED)
set_property(TARGET avdevice PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/avdevice.lib"
			)

add_library(avfilter SHARED IMPORTED)
set_property(TARGET avfilter PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/avfilter.lib"
			)

add_library(avformat SHARED IMPORTED)
set_property(TARGET avformat PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/avformat.lib"
			)
			
add_library(avutil SHARED IMPORTED)
set_property(TARGET avutil PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/avutil.lib"
			)

add_library(postproc SHARED IMPORTED)
set_property(TARGET postproc PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/postproc.lib"
			)
			
add_library(swresample SHARED IMPORTED)
set_property(TARGET swresample PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/swresample.lib"
			)

add_library(swscale SHARED IMPORTED)
set_property(TARGET swscale PROPERTY
			IMPORTED_IMPLIB "${ffmpeg_DIR}/lib/swscale.lib"
			)

set(ffmpeg avcodec avdevice avfilter avformat avutil postproc swresample swscale)
	
####################### source #######################
	
set(CODEC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list_source_by_dir(${CODEC_DIR} SOURCE_FILE)

source_group_by_files(SOURCE_FILE)

add_library(${PRO_NAME} STATIC ${SOURCE_FILE})

target_link_libraries(${PRO_NAME} PUBLIC ${ffmpeg} yuv ${OpenCV_LIBS} SDL2)
target_include_directories(${PRO_NAME} PUBLIC 
${CODEC_DIR} ${ffmpeg_INCLUDE} ${libyuv_INCLUDE} ${OpenCV_INCLUDE_DIRS})

####################### example #######################
add_executable(${PRO_NAME}-example01 ${CODEC_DIR}/example/example01.cpp)
target_link_libraries(${PRO_NAME}-example01 PRIVATE ${PRO_NAME})

add_executable(${PRO_NAME}-example02 ${CODEC_DIR}/example/example02.cpp)
target_link_libraries(${PRO_NAME}-example02 PRIVATE ${PRO_NAME})


#file(COPY ${ffmpeg_DIR}/bin/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY ${OpenCV_DIR}/x64/vc15/bin/opencv_world451d.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
