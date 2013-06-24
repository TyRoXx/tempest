file(GLOB_RECURSE files "*.hpp" "*.cpp")
add_library(tempest ${files})
target_link_libraries(tempest http)

install(TARGETS tempest ARCHIVE DESTINATION lib)
