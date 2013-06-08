cmake_minimum_required(VERSION 2.8)

project(tempest)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -pedantic")
endif()

#Boost is used for testing, networking and multi-threading
find_package(Boost REQUIRED unit_test_framework system thread program_options)
include_directories(${Boost_INCLUDE_DIR})

#Boost test requires this definition if linked dynamically which we want to do
add_definitions(-DBOOST_TEST_DYN_LINK)

include_directories(.)

add_subdirectory(http)
add_subdirectory(test)

#the actual web server application
add_executable(tempest tempest.cpp)
target_link_libraries(tempest ${Boost_LIBRARIES} http)
