
# =============== deps ==============
find_package(Vulkan REQUIRED shaderc_combined)

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
add_subdirectory("lib/vma-hpp")
add_subdirectory("lib/imgui")
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
target_link_libraries(geg PUBLIC Vulkan::shaderc_combined)
target_link_libraries(geg PUBLIC spdlog::spdlog)
target_link_libraries(geg PUBLIC glfw)
target_link_libraries(geg PUBLIC glm)
target_link_libraries(geg PUBLIC VulkanMemoryAllocator)
target_link_libraries(geg PUBLIC imgui)
target_link_libraries(geg PUBLIC assimp)

# vma-hpp
target_include_directories(geg PUBLIC "lib/vma-hpp/include")

# pre compiled header
target_precompile_headers(geg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")
# ==================================

# ============== copy standard assets =============
add_custom_command(
  TARGET geg POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/default-assets"
  "${CMAKE_BINARY_DIR}/assets"
)
# ================================================

# ============== include =============
target_include_directories(geg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
#=====================================
