cmake_minimum_required(VERSION 3.20.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(fastDDS_Robot LANGUAGES CXX)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall)

find_package (Curses)

find_package(fastcdr REQUIRED)
find_package(fastdds QUIET)

include_directories(${CMAKE_SOURCE_DIR})

include(GNUInstallDirs)

###########
# DDS messages

set(LIBDDSIDL ${CMAKE_SOURCE_DIR}/brake.idl  ${CMAKE_SOURCE_DIR}/steering.idl  ${CMAKE_SOURCE_DIR}/throttle.idl ${CMAKE_SOURCE_DIR}/button.idl)

execute_process(COMMAND fastddsgen ${LIBDDSIDL})

file(GLOB DDS_SOURCES_CXX "*.cxx")
add_library(FastDDSMsg STATIC ${DDS_SOURCES_CXX})

target_link_libraries(FastDDSMsg fastdds fastcdr)

add_custom_target(
    ddsmessages
    COMMAND fastddsgen ${LIBDDSIDL}
)

add_dependencies(FastDDSMsg ddsmessages)


##################
# Main stuff


add_executable (fastDDSrobot fastDDSrobot.cpp)
target_link_libraries(fastDDSrobot zetabot ncurses rt FastDDSMsg)

##################
# Motor tester

add_executable (testIO testIO.cpp)
target_link_libraries(testIO zetabot ncurses rt gpiodcxx)
