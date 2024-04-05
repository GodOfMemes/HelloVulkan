cmake_minimum_required (VERSION 3.8)

project (HelloVulkan)

find_package(Vulkan REQUIRED)
find_package(glslang QUIET)

include(cmake/Helper.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# All external libraries from git modules
add_subdirectory("${PROJECT_SOURCE_DIR}/vendor")

# The actual source code
add_subdirectory("${PROJECT_SOURCE_DIR}/HelloVulkan")
handle_assets(SampleResources ${PROJECT_SOURCE_DIR}/Assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets)
add_subdirectory("${PROJECT_SOURCE_DIR}/Samples")

# Put CMake targets to a folder called "CMakeTargets"
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
set_property(TARGET uninstall PROPERTY FOLDER "CMakeTargets")