cmake_minimum_required(VERSION 2.8)

project(tempest)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -pedantic")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -DBOOST_ASIO_SEPARATE_COMPILATION")
endif()

if(MSVC)
	set(Boost_USE_STATIC_LIBS ON)

	#for Boost.ASIO
	add_definitions(/D_WIN32_WINDOWS)
endif()

#Boost is used for testing, networking and multi-threading
find_package(Boost REQUIRED date_time chrono unit_test_framework system
             thread program_options filesystem iostreams)
include_directories(${Boost_INCLUDE_DIR})

#Boost test requires this definition if linked dynamically which we want to do
if(UNIX)
	add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)

include_directories(.)

add_subdirectory(http)
add_subdirectory(tempest)
add_subdirectory(test)

#the actual web server application
add_executable(tempestd tempestd.cpp)
target_link_libraries(tempestd tempest ${Boost_LIBRARIES})

file(GLOB headers "http/*.hpp")
install(FILES ${headers} DESTINATION "include/http")

file(GLOB headers "tempest/*.hpp")
install(FILES ${headers} DESTINATION "tempest/http")
