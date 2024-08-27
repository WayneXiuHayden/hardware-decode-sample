cmake_minimum_required(VERSION 3.12)

project(seek_min_ex)


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
        gstreamer-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${OpenCV_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  )
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${OpenCV_LIBRARIES}
  ${GST_LIBRARIES}
  Threads::Threads
  )
