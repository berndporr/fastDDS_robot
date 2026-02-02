cmake_minimum_required(VERSION 3.20.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(rt_bp_robot LANGUAGES CXX)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall)

find_package (Curses)

find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

include(GNUInstallDirs)

###########
# DDS

set(LIBDDSIDL ${CMAKE_SOURCE_DIR}/../brake.idl  ${CMAKE_SOURCE_DIR}/../steering.idl  ${CMAKE_SOURCE_DIR}/../throttle.idl ${CMAKE_SOURCE_DIR}/../button.idl)

execute_process(COMMAND fastddsgen ${LIBDDSIDL})

add_library(FastDDSMsg STATIC
	      brake.cxx brakePubSubTypes.cxx
	      steering.cxx steeringPubSubTypes.cxx
	      throttle.cxx throttlePubSubTypes.cxx
	      button.cxx buttonPubSubTypes.cxx
	      )

target_link_libraries(FastDDSMsg fastcdr fastrtps)

add_custom_target(
    ddsmessages
    COMMAND fastddsgen ${LIBDDSIDL}
)

add_dependencies(FastDDSMsg ddsmessages)

# Publisher
add_executable(steering_wheel steering_wheel.cpp)
target_link_libraries(steering_wheel fastcdr fastrtps FastDDSMsg)
