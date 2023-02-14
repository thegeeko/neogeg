# ========= setup ================
set(GCC_COVERAGE_COMPILE_FLAGS "")
set(MSVC_COVERAGE_COMPILE_FLAGS "")

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
else()
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${MSVC_COVERAGE_COMPILE_FLAGS}")
endif()
# ================================

# =============== deps ==============
find_package(Vulkan REQUIRED shaderc_combined)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory("lib/glfw")


add_subdirectory("lib/spdlog")
add_subdirectory("lib/glm")
add_subdirectory("lib/vma-hpp")

# ===================================

# =============== geg ==============
# find all source files and headers
file(
	GLOB_RECURSE # recursive
	GEG_SRC # variable to store the source files and headers
	CONFIGURE_DEPENDS # make a dependency
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
add_library(geg STATIC ${GEG_SRC})

# link dependencies
target_link_libraries(geg PUBLIC Vulkan::Vulkan)
target_link_libraries(geg PUBLIC Vulkan::shaderc_combined)
target_link_libraries(geg PUBLIC spdlog::spdlog)
target_link_libraries(geg PUBLIC glfw)
target_link_libraries(geg PUBLIC glm)
target_link_libraries(geg PUBLIC VulkanMemoryAllocator)

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
  "${CMAKE_CURRENT_BINARY_DIR}/../assets"
)
# ================================================

# ============== include =============
target_include_directories(geg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
#=====================================
