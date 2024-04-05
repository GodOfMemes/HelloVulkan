# This CMake file has only been tested on Windows 11 and Ubuntu 22.04 LTS

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(HelloVulkan STATIC
    ${CPP_FILES}
)

target_compile_definitions(HelloVulkan PUBLIC
    _CRT_SECURE_NO_WARNINGS
    TRACY_ENABLE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    VK_NO_PROTOTYPES
)

if(MSVC)
    # Tracy does not like /ZI so set it to /Zi
    target_compile_options(HelloVulkan PUBLIC "/Zi")
endif()  

# Include directories
target_include_directories(HelloVulkan PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Set dependencies
target_link_libraries(HelloVulkan PUBLIC Deps)