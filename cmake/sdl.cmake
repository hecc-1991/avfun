set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

#set(sdl_DIR ${THIRD_PARTY_DIR}/SDL2)
#set(sdl_INCLUDE ${THIRD_PARTY_DIR}/SDL2/include)
#add_subdirectory(${sdl_DIR} ${sdl_DIR}/build)

set(CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR}/SDL2_build)
find_package(SDL2)
if(SDL2_FOUND)
else(SDL2_FOUND)
    message(FATAL_ERROR ”SDL2 library not found”)
endif(SDL2_FOUND)