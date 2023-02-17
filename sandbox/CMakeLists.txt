# =============== geg ===============
# find all source files and headers
file(
	GLOB_RECURSE # recursive
	SANDBOX_SRC # variable to store the source files and headers
	CONFIGURE_DEPENDS # make a dependency
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
add_executable(sandbox ${SANDBOX_SRC})
# ===================================

target_link_libraries(sandbox PRIVATE geg)

set_target_properties(
  sandbox
  PROPERTIES OUTPUT_NAME_DEBUG
  sandbox_Debug
)

set_target_properties(
  sandbox
  PROPERTIES OUTPUT_NAME_RELEASE
  sandbox_Release
)

set_target_properties(
  sandbox
  PROPERTIES OUTPUT_NAME_RELWITHDEBINFO
  sandbox_ReleaseDebInfo
)


if(MSVC)
  add_definitions(-D_CONSOLE)
  set_property(
    TARGET
    sandbox
    PROPERTY
    VS_DEBUGGER_WORKING_DIRECTORY
    "${CMAKE_BINARY_DIR}"
  )
endif()

