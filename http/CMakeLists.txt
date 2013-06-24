file(GLOB files "*.hpp" "*.cpp")
add_library(http ${files})

install(TARGETS http ARCHIVE DESTINATION lib)
