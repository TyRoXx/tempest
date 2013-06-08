
file(GLOB files "*.cpp")
add_executable(test ${files})
target_link_libraries(test ${Boost_LIBRARIES} http)
