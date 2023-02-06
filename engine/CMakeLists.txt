# ========= setup ================
set(GCC_COVERAGE_COMPILE_FLAGS "-fPIC")
set(MSVC_COVERAGE_COMPILE_FLAGS "")

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
else()
	set(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${MSVC_COVERAGE_COMPILE_FLAGS}")
endif()
# ================================

# =============== geg ==============
# find all source files and headers
file(
	GLOB_RECURSE # recursive
	GEG_SRC # variable to store the source files and headers
	CONFIGURE_DEPENDS # make a dependency
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
add_library(geg STATIC ${GEG_SRC})
# ==================================

# ============== include =============
target_include_directories(geg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
#=====================================
