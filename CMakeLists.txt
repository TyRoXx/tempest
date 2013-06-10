cmake_minimum_required(VERSION 2.8)

project(tempest)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -pedantic")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic")
endif()

#Boost is used for testing, networking and multi-threading
find_package(Boost REQUIRED unit_test_framework system thread program_options filesystem iostreams)
include_directories(${Boost_INCLUDE_DIR})

#Boost test requires this definition if linked dynamically which we want to do
add_definitions(-DBOOST_TEST_DYN_LINK)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)

include_directories(.)

add_subdirectory(http)
add_subdirectory(tempest)
add_subdirectory(test)

#the actual web server application
add_executable(tempestd tempestd.cpp)
target_link_libraries(tempestd tempest ${Boost_LIBRARIES})
