cmake_minimum_required(VERSION 3.17)
project(distplusplus)

# see https://gitlab.kitware.com/cmake/cmake/-/issues/18090
macro(obj_propagate_targets name)
	target_sources(${name} INTERFACE $<TARGET_OBJECTS:${name}>)
endmacro()

option(COVERAGE "build with coverage data" OFF)
option(USE_DEFAULT_FLAGS "use default compiler flags" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# no GNU C
set(CMAKE_CXX_EXTENSIONS OFF)
# We don't use aliasing or interposition'
string(APPEND CMAKE_CXX_FLAGS " -fno-semantic-interposition -fstrict-aliasing -Wall")
if(COVERAGE)
	if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		message(FATAL_ERROR "coverage requires clang")
	endif()
	string(PREPEND CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping ")
endif()

if(USE_DEFAULT_FLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=thin -fwhole-program-vtables")
		string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=auto")
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g3 -fno-omit-frame-pointer")
endif()


include(FindPkgConfig)
include(CheckIncludeFileCXX)
include(CTest)

pkg_search_module(grpc REQUIRED IMPORTED_TARGET GLOBAL grpc++)
pkg_search_module(protobuf REQUIRED IMPORTED_TARGET GLOBAL protobuf>=3.15)

add_compile_definitions("BOOST_ALL_DYN_LINK=1")
find_package(Boost REQUIRED COMPONENTS filesystem log)

CHECK_INCLUDE_FILE_CXX("toml++/toml.h" HAVE_TOMLPLUSPLUS)
if(NOT HAVE_TOMLPLUSPLUS)
	message(STATUS "system tomlplusplus not found, using bundled version")
	include_directories("third_party/tomlplusplus/include")
endif()

execute_process(
	COMMAND bash -c "LANG=en ${CMAKE_CXX_COMPILER} -v 2> >(grep Target | cut -d ' ' -f 2)"
	OUTPUT_VARIABLE DISTPLUSPLUS_BUILD_TARGET)
if(NOT DISTPLUSPLUS_BUILD_TARGET)
	message(FATAL_ERROR "could not determine build target from $CXX -v")
endif()
add_compile_definitions("DISTPLUSPLUS_BUILD_TARGET=${DISTPLUSPLUS_BUILD_TARGET}")

include_directories("src")
include_directories(".")
add_subdirectory("protos")
add_subdirectory("src/common")
add_subdirectory("src/client")
add_subdirectory("src/server")

if(BUILD_TESTING)
	add_subdirectory("test")
endif()
