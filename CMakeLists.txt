cmake_minimum_required(VERSION 3.23.2 FATAL_ERROR)

project(fingerprint_win CXX)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(CMAKE_CXX_STANDARD 17)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-Wall -g -O0)
else()
	add_definitions(-Wall -O3)
endif()

include_directories("${CMAKE_SOURCE_DIR}/include/")

add_executable(
		fingerprint_win
		${CMAKE_SOURCE_DIR}/src/main.cpp
		${CMAKE_SOURCE_DIR}/src/Fingerprint.cpp)

target_link_libraries(fingerprint_win iphlpapi.lib )