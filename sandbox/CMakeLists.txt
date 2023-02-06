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