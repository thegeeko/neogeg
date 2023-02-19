cmake_minimum_required(VERSION 3.25)
project(Neogeg)

# ========= setup ================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
set(CMAKE_BUILD_TYPE Debug)

set(GCC_COVERAGE_COMPILE_FLAGS "")
set(MSVC_COVERAGE_COMPILE_FLAGS "/MP")

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
else()
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${MSVC_COVERAGE_COMPILE_FLAGS}")
endif()
# ================================

include(CMake/CommonMacros.cmake)

add_subdirectory(engine)
add_subdirectory(sandbox)

# link compile commands in root if it's not visual studio
if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  execute_process(
  	COMMAND ${CMAKE_COMMAND} -E create_symlink
  		${CMAKE_BINARY_DIR}/compile_commands.json
  		${CMAKE_SOURCE_DIR}/compile_commands.json
  )
endif()
