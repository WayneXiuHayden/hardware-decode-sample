cmake_minimum_required(VERSION 3.10)


project(min-ex)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
  gstreamer-1.0>=1.4
  gstreamer-video-1.0>=1.4
  gstreamer-app-1.0>=1.4
  )


add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${GST_INCLUDE_DIRS}
  )
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${GST_LIBRARIES}
  )
