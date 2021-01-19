set(PRO_NAME codec)

## support c++17
set(CMAKE_CXX_STANDARD 17)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)


####################### opencv #######################
#set(opencv_DIR ${THIRD_PARTY_DIR}/opencv)
#add_subdirectory(${opencv_DIR} ${opencv_DIR}/build EXCLUDE_FROM_ALL)


####################### libyuv #######################
set(libyuv_DIR ${THIRD_PARTY_DIR}/libyuv)
set(libyuv_INCLUDE ${THIRD_PARTY_DIR}/libyuv/include)
add_subdirectory(${libyuv_DIR} ${libyuv_DIR}/build)


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

target_link_libraries(${PRO_NAME} PUBLIC ${ffmpeg} yuv)
target_include_directories(${PRO_NAME} PUBLIC ${CODEC_DIR} ${ffmpeg_INCLUDE} ${libyuv_INCLUDE})

####################### example #######################
add_executable(${PRO_NAME}-example01 ${CODEC_DIR}/example/example01.cpp)
target_link_libraries(${PRO_NAME}-example01 PRIVATE ${PRO_NAME})

#add_custom_target(copy_dll
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/avcodec-58.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/avdevice-58.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/avfilter-7.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/avformat-58.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/avutil-56.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/postproc-55.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/swresample-3.dll ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ffmpeg_DIR}/bin/swscale-5.dll ${CMAKE_CURRENT_BINARY_DIR}
#)

#add_dependencies(${PRO_NAME}-example01 copy_dll)
