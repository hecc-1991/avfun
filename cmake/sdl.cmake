set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

set(sdl_DIR ${THIRD_PARTY_DIR}/SDL2)
set(sdl_INCLUDE ${THIRD_PARTY_DIR}/SDL2/include)
add_subdirectory(${sdl_DIR} ${sdl_DIR}/build)