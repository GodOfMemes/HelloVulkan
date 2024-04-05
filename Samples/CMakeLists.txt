
file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_executable(Sample ${CPP_FILES})
add_dependencies(Sample SampleResources)

# Include directories
target_include_directories(Sample PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Set dependencies
target_link_libraries(Sample PRIVATE HelloVulkan)