
# =============== deps ==============
find_package(Vulkan REQUIRED glslang SPIRV-Tools)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory("lib/glfw")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
add_subdirectory("lib/assimp")

add_subdirectory("lib/spdlog")
add_subdirectory("lib/glm")
add_subdirectory("lib/vma")
add_subdirectory("lib/imgui")
add_subdirectory("lib/stb")
add_subdirectory("lib/entt")
# ===================================

# =============== geg ==============
# find all source files and headers
file(
        GLOB_RECURSE # recursive
        GEG_SRC # variable to store the source files and headers
        CONFIGURE_DEPENDS # make a dependency
        RELATIVE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(
        GLOB_RECURSE # recursive
        GEG_HEADERS # variable to store the source files and headers
        CONFIGURE_DEPENDS # make a dependency
        RELATIVE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

SETUP_GROUPS("${GEG_SRC}")
SETUP_GROUPS("${GEG_HEADERS}")

add_library(geg STATIC ${GEG_SRC} ${GEG_HEADERS})

# link dependencies
target_link_libraries(geg PUBLIC Vulkan::Vulkan)
target_link_libraries(geg PUBLIC Vulkan::glslang)
target_link_libraries(geg PUBLIC Vulkan::SPIRV-Tools)
target_link_libraries(geg PUBLIC spdlog::spdlog)
target_link_libraries(geg PUBLIC glfw)
target_link_libraries(geg PUBLIC glm)
target_link_libraries(geg PUBLIC GPUOpen::VulkanMemoryAllocator)
target_link_libraries(geg PUBLIC imgui)
target_link_libraries(geg PUBLIC stb)
target_link_libraries(geg PUBLIC assimp)
target_link_libraries(geg PUBLIC EnTT)


# pre compiled header
target_precompile_headers(geg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")
# ==================================

# ============== include =============
target_include_directories(geg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
#=====================================
