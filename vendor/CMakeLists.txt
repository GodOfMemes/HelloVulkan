find_package(Vulkan REQUIRED)

# Compile volk and tracy as static libraries
add_subdirectory(volk)
add_subdirectory(tracy)

# GLFW
set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(glfw)

# Assimp
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PBD OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
add_subdirectory(assimp)

# ImGui
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC 
    "$ENV{VULKAN_SDK}/Include"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui"
    "${CMAKE_CURRENT_LIST_DIR}/glfw/include"
)
target_compile_definitions(imgui PRIVATE VK_NO_PROTOTYPES)  # Important because of volk is used
target_sources(imgui PRIVATE 
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.h"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.h"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.h"
    "${CMAKE_CURRENT_LIST_DIR}/ImGuizmo/ImGuizmo.h"

    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_demo.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_draw.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_tables.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGuizmo/ImGuizmo.cpp"
)

# Group libraries into a folder
set_property(TARGET glfw PROPERTY FOLDER "External")
set_property(TARGET volk PROPERTY FOLDER "External")
set_property(TARGET assimp PROPERTY FOLDER "External")
set_property(TARGET imgui PROPERTY FOLDER "External")
set_property(TARGET zlibstatic PROPERTY FOLDER "External")
set_property(TARGET TracyClient PROPERTY FOLDER "External")
set_property(TARGET update_mappings PROPERTY FOLDER "External") # glfw

if(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External")
endif()

# Set dependencies
add_library(Deps INTERFACE)
target_link_libraries(Deps INTERFACE assimp imgui glfw TracyClient volk)

target_include_directories(Deps INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/stb"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends"
    "${CMAKE_CURRENT_LIST_DIR}/ImGuizmo")

if(glslang_FOUND)
    target_link_libraries(Deps INTERFACE
            glslang::glslang
            glslang::SPIRV
            glslang::glslang-default-resource-limits
    )
else()
    if(WIN32)
        get_filename_component(VK_SDK_PATH ${Vulkan_LIBRARY} DIRECTORY) # Some CMake Generators don't play nice with $ENV{VULKAN_SDK}
        get_filename_component(VK_SDK_PATH ${VK_SDK_PATH} DIRECTORY)
        target_link_libraries(Deps INTERFACE
                $<$<CONFIG:Debug>:
                "${VK_SDK_PATH}/Lib/GenericCodeGend.lib"
                "${VK_SDK_PATH}/Lib/glslangd.lib"
                "${VK_SDK_PATH}/Lib/glslang-default-resource-limitsd.lib"
                "${VK_SDK_PATH}/Lib/MachineIndependentd.lib"
                "${VK_SDK_PATH}/Lib/SPIRVd.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Toolsd.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools-optd.lib"
                "${VK_SDK_PATH}/Lib/OSDependentd.lib"
                >
                $<$<CONFIG:Release>:
                "${VK_SDK_PATH}/Lib/GenericCodeGen.lib"
                "${VK_SDK_PATH}/Lib/glslang.lib"
                "${VK_SDK_PATH}/Lib/glslang-default-resource-limits.lib"
                "${VK_SDK_PATH}/Lib/MachineIndependent.lib"
                "${VK_SDK_PATH}/Lib/SPIRV.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools-opt.lib"
                "${VK_SDK_PATH}/Lib/OSDependent.lib"
                >
        )
    endif()
endif()