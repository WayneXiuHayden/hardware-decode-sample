cmake_minimum_required(VERSION 3.10)
project(vp9-seek)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-video-1.0)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

add_executable(vp9-seek main.cpp)
target_link_libraries(vp9-seek ${GST_LIBRARIES})
