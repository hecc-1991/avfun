cmake_minimum_required(VERSION 3.10)

project(avfun-master)

####################### function #######################

## 建立文件目录结构
macro(source_group_by_files SOURCE_FILE)

	set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
	foreach(sgbd_file ${${SOURCE_FILE}})
		string(REGEX REPLACE "${sgbd_cur_dir}/(.*)" "\\1" sgbd_fpath ${sgbd_file})
		string(REGEX REPLACE "(.*)/.*" "\\1" sgbd_group_name ${sgbd_fpath})
		string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
		string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
		if(sgbd_nogroup)
			set(sgbd_group_name "\\")
		endif(sgbd_nogroup)
		source_group(${sgbd_group_name} FILES ${sgbd_file})
	endforeach(sgbd_file)
	
endmacro()

## 查找当前目录(c/cpp)文件 
macro(list_source_by_dir SOURCE_DIR SOURCE_FILE)
file(GLOB CUR_HEADER RELATIVE ${SOURCE_DIR} *.hh *.h *.hpp)
file(GLOB CUR_SOURCE *.cc *.c *.cpp)
set(SOURCE_FILE ${CUR_HEADER} ${CUR_SOURCE})
endmacro()

add_subdirectory(avfun)

