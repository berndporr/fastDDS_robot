cmake_minimum_required(VERSION 3.20.0)

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
find_package(fastdds QUIET)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include path: ${OpenCV_INCLUDE_DIRS}")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${TORCH_INCLUDE_DIRS})

include(GNUInstallDirs)

###########
# DDS

if(fastdds_FOUND)

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

endif()

##################
# Main stuff


if(fastdds_FOUND)
    add_executable (rt_bp_robot rt_bp_robot.cpp)
    target_link_libraries(rt_bp_robot zetabot ncurses rt FastDDSMsg)
endif()

add_executable (testIO testIO.cpp)
target_link_libraries(testIO zetabot ncurses rt gpiodcxx)
add_executable (testMotor testMotor.cpp)
target_link_libraries(testMotor zetabot ncurses rt gpiodcxx)
