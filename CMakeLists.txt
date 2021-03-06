cmake_minimum_required(VERSION 2.8.3)
project(ecn_visualodom)

add_compile_options(-std=c++11)

set(CMAKE_BUILD_TYPE DEBUG)
find_package(VISP REQUIRED)
find_package(OpenCV 3 REQUIRED)

###########

## Specify additional locations of header files
include_directories(
  include
  ${VISP_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

add_definitions(-DOGRE_RESSOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/ogre-simulator")

## Visual Odometry
add_executable(visualodom vo.cpp
                include/ecn_visualodom/ar_display.h
                include/ecn_visualodom/video_loop.h
                include/ecn_visualodom/visual_odom.h src/visual_odom.cpp
                )
target_link_libraries(visualodom ${VISP_LIBRARIES}  ${OpenCV_LIBS})
