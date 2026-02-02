cmake_minimum_required(VERSION 3.10.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(rt_bp_robot LANGUAGES CXX)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall)

find_package (Curses)

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)
find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)
pkg_check_modules(LIBCAMERA IMPORTED_TARGET libcamera)


message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include path: ${OpenCV_INCLUDE_DIRS}")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${TORCH_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})

include(GNUInstallDirs)

###########
# DDS

set(LIBDDSIDL ${CMAKE_SOURCE_DIR}/brake.idl  ${CMAKE_SOURCE_DIR}/steering.idl  ${CMAKE_SOURCE_DIR}/throttle.idl ${CMAKE_SOURCE_DIR}/button.idl)

execute_process(COMMAND fastddsgen ${LIBDDSIDL})

file(GLOB DDS_SOURCES_CXX "*.cxx")
add_library(FastDDSMsg STATIC
		       ${DDS_SOURCES_CXX})

target_link_libraries(FastDDSMsg fastdds fastcdr)

add_custom_target(
    ddsmessages
    COMMAND fastddsgen ${LIBDDSIDL}
)

add_dependencies(FastDDSMsg ddsmessages)

###################
# Torch / RT learner
add_library(rt_bp STATIC rt_bp.cpp)
target_link_libraries(rt_bp
  ${TORCH_LIBRARIES} 
  ${OpenCV_LIBRARIES}
      extension_tensor
    extension_module
    extension_data_loader
    extension_flat_tensor
    executorch
    executorch_core

    # XNNPACK backend
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,xnnpack_backend>"

    # XNNPACK dependencies
    XNNPACK

    xnnpack-microkernels-prod

    pthreadpool
    extension_threadpool

    quantized_kernels
    quantized_ops_lib
    kernels_util_all_deps
    cpuinfo
    kleidiai
  )


add_executable (test_rt_bp test_rt_bp.cpp)
target_link_libraries(test_rt_bp rt_bp)


##################
# Main stuff


if(LIBCAMERA_FOUND)
  message(STATUS "libcamera found — building RPI stuff")
  include_directories(${LIBCAMERA_INCLUDE_DIRS})
  add_compile_definitions(HAVE_LIBCAMERA)

  add_executable (rt_bp_robot rt_bp_robot.cpp)
  target_link_libraries(rt_bp_robot rt_bp alphabot ncurses rt FastDDSMsg ${TORCH_LIBRARIES} ${OpenCV_LIBRARIES} PkgConfig::LIBCAMERA cam2opencv gpiodcxx)

  add_executable (manual_robot manual_robot.cpp)
  target_link_libraries(manual_robot alphabot ncurses rt FastDDSMsg ${OpenCV_LIBRARIES} PkgConfig::LIBCAMERA cam2opencv gpiodcxx)

  add_executable (testIO testIO.cpp)
  target_link_libraries(testIO alphabot ncurses rt gpiodcxx)
  add_executable (testMotor testMotor.cpp)
  target_link_libraries(testMotor alphabot ncurses rt gpiodcxx)
else()
  message(STATUS "libcamera not found — disabling rt_bp_robot")
endif()

