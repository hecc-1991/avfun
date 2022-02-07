set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

set(libyuv_DIR ${THIRD_PARTY_DIR}/libyuv)
set(libyuv_INCLUDE ${THIRD_PARTY_DIR}/libyuv/include)
add_subdirectory(${libyuv_DIR} ${libyuv_DIR}/build EXCLUDE_FROM_ALL)