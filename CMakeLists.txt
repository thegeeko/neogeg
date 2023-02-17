cmake_minimum_required(VERSION 3.24)
project(Neogeg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
set(CMAKE_BUILD_TYPE Debug)
include(CMake/CommonMacros.cmake)

add_subdirectory(engine)
add_subdirectory(sandbox)


if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
  execute_process(
  	COMMAND ${CMAKE_COMMAND} -E create_symlink
  		${CMAKE_BINARY_DIR}/compile_commands.json
  		${CMAKE_SOURCE_DIR}/compile_commands.json
  )
endif()
