set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)


if(AVF_ANDROID)
    set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/android/sdk/native/jni)
else()
    set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv/mac/lib/cmake/opencv4)
endif()

find_package(OpenCV REQUIRED)

if(OpenCV_FOUND)
    message(STATUS "------------OpenCV ${OpenCV_VERSION} fonud-----------")
    message(STATUS "-- OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "-- OpenCV_LIBS = ${OpenCV_LIBS}")
else(OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found")
endif(OpenCV_FOUND)