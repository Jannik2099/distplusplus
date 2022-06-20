cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21)
project(distplusplus)

include(FindPkgConfig)
include(CheckIncludeFileCXX)
include(CTest)
include(ExternalProject)

# see https://gitlab.kitware.com/cmake/cmake/-/issues/18090
macro(obj_propagate_targets name)
    target_sources(${name} INTERFACE $<TARGET_OBJECTS:${name}>)
endmacro()

macro(copy_shim_target target src dest dep)
    add_custom_command(
        OUTPUT ${dest}
        COMMAND cp ${src} ${dest}
        DEPENDS ${dep}
    )
    add_custom_target(${target}__ DEPENDS ${dest})
    add_dependencies(${target} ${target}__)
    set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${dest})
endmacro()

option(COVERAGE "build with coverage data" OFF)
option(USE_DEFAULT_FLAGS "use default compiler flags" ON)
option(SYSTEM_LMDBXX "use system lmdb++" ON)
option(SYSTEM_BOOST "use system boost" ON)
option(SYSTEM_GRPC "use system protobuf & grpc" ON)
set(ZSTD ON) # can't be bothered to write proper detection logic
pkg_search_module(zstd REQUIRED IMPORTED_TARGET libzstd)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# no GNU C
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
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
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g3 -fno-omit-frame-pointer")
endif()

pkg_search_module(lmdb REQUIRED IMPORTED_TARGET GLOBAL lmdb)
if(SYSTEM_LMDBXX)
    CHECK_INCLUDE_FILE_CXX(lmdb++.h HAS_LMDBXX_H)
    if(NOT HAS_LMDBXX_H)
    mesage(FATAL_ERROR "required header lmdb++.h not found")
    endif()
else()
    include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lmdbxx)
endif()

add_custom_target(protoc_ready)
if(SYSTEM_GRPC)
    pkg_search_module(grpc++ REQUIRED IMPORTED_TARGET GLOBAL grpc++)
    pkg_search_module(protobuf REQUIRED IMPORTED_TARGET GLOBAL protobuf>=3.15)
    find_program(protoc protoc REQUIRED)
    find_program(grpc_cpp_plugin grpc_cpp_plugin REQUIRED)
else()
    # can't be bothered to sort the static archives
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld")
    ExternalProject_Add(grpc-third_party
        PREFIX
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc
        SOURCE_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc
        INSTALL_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_INSTALL_LIBDIR=lib
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
    set(protoc ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install/bin/protoc)
    set(grpc_cpp_plugin ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install/bin/grpc_cpp_plugin)
    add_dependencies(protoc_ready grpc-third_party)
    include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install/include)
    
    macro(register_grpc_lib name)
        add_library(${name} STATIC IMPORTED)
        copy_shim_target(${name} ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install/lib/lib${name}.a ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install-fixed/lib/lib${name}.a grpc-third_party)
        list(APPEND grpc_libs ${name})
    endmacro()

    # it was easier to create this via find | echo than think about a better solution to do it at runtime ¯\_(ツ)_/¯
    # for i in $(find . -type f); do file=${i#./}; if [[ $file == *.a ]]; then file2=${file%.a}; file3=${file2#lib}; echo "register_grpc_lib($file3)"; fi; done
register_grpc_lib(cares)
register_grpc_lib(re2)
register_grpc_lib(ssl)
register_grpc_lib(crypto)
register_grpc_lib(z)
register_grpc_lib(address_sorting)
register_grpc_lib(gpr)
register_grpc_lib(grpc)
register_grpc_lib(grpc_unsecure)
register_grpc_lib(grpc++)
register_grpc_lib(grpc++_alts)
register_grpc_lib(grpc++_error_details)
register_grpc_lib(grpc++_reflection)
register_grpc_lib(grpc++_unsecure)
register_grpc_lib(grpc_plugin_support)
register_grpc_lib(grpcpp_channelz)
register_grpc_lib(upb)
register_grpc_lib(absl_log_severity)
register_grpc_lib(absl_raw_logging_internal)
register_grpc_lib(absl_spinlock_wait)
register_grpc_lib(absl_malloc_internal)
register_grpc_lib(absl_base)
register_grpc_lib(absl_throw_delegate)
register_grpc_lib(absl_scoped_set_env)
register_grpc_lib(absl_strerror)
register_grpc_lib(absl_hashtablez_sampler)
register_grpc_lib(absl_raw_hash_set)
register_grpc_lib(absl_stacktrace)
register_grpc_lib(absl_symbolize)
register_grpc_lib(absl_examine_stack)
register_grpc_lib(absl_failure_signal_handler)
register_grpc_lib(absl_debugging_internal)
register_grpc_lib(absl_demangle_internal)
register_grpc_lib(absl_leak_check)
register_grpc_lib(absl_leak_check_disable)
register_grpc_lib(absl_flags_program_name)
register_grpc_lib(absl_flags_config)
register_grpc_lib(absl_flags_marshalling)
register_grpc_lib(absl_flags_commandlineflag_internal)
register_grpc_lib(absl_flags_commandlineflag)
register_grpc_lib(absl_flags_private_handle_accessor)
register_grpc_lib(absl_flags_reflection)
register_grpc_lib(absl_flags_internal)
register_grpc_lib(absl_flags)
register_grpc_lib(absl_flags_usage_internal)
register_grpc_lib(absl_flags_usage)
register_grpc_lib(absl_flags_parse)
register_grpc_lib(absl_hash)
register_grpc_lib(absl_city)
register_grpc_lib(absl_low_level_hash)
register_grpc_lib(absl_int128)
register_grpc_lib(absl_exponential_biased)
register_grpc_lib(absl_periodic_sampler)
register_grpc_lib(absl_random_distributions)
register_grpc_lib(absl_random_seed_gen_exception)
register_grpc_lib(absl_random_seed_sequences)
register_grpc_lib(absl_random_internal_seed_material)
register_grpc_lib(absl_random_internal_pool_urbg)
register_grpc_lib(absl_random_internal_platform)
register_grpc_lib(absl_random_internal_randen)
register_grpc_lib(absl_random_internal_randen_slow)
register_grpc_lib(absl_random_internal_randen_hwaes)
register_grpc_lib(absl_random_internal_randen_hwaes_impl)
register_grpc_lib(absl_random_internal_distribution_test_util)
register_grpc_lib(absl_status)
register_grpc_lib(absl_statusor)
register_grpc_lib(absl_strings)
register_grpc_lib(absl_strings_internal)
register_grpc_lib(absl_str_format_internal)
register_grpc_lib(absl_cord_internal)
register_grpc_lib(absl_cordz_functions)
register_grpc_lib(absl_cordz_handle)
register_grpc_lib(absl_cordz_info)
register_grpc_lib(absl_cordz_sample_token)
register_grpc_lib(absl_cord)
register_grpc_lib(absl_graphcycles_internal)
register_grpc_lib(absl_synchronization)
register_grpc_lib(absl_time)
register_grpc_lib(absl_civil_time)
register_grpc_lib(absl_time_zone)
register_grpc_lib(absl_bad_any_cast_impl)
register_grpc_lib(absl_bad_optional_access)
register_grpc_lib(absl_bad_variant_access)

    target_link_libraries(grpc++ INTERFACE ${grpc_libs})

    add_library(protobuf STATIC IMPORTED)
    copy_shim_target(protobuf ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install/lib/libprotobuf.a ${CMAKE_CURRENT_BINARY_DIR}/third_party/grpc/install-fixed/lib/libprotobuf.a grpc-third_party)

    add_library(PkgConfig::grpc++ ALIAS grpc++)
    add_library(PkgConfig::protobuf ALIAS protobuf)
endif()

if(SYSTEM_BOOST)
    add_compile_definitions("BOOST_ALL_DYN_LINK=1")
    # at least 1.70 due to zstd, no clue about the rest
    find_package(Boost 1.70 REQUIRED COMPONENTS filesystem log program_options iostreams)
    add_custom_target(Boost)
else()
    add_custom_target(Boost_Build
        COMMAND
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost/tools/build/src/engine/build.sh
    )
    add_custom_target(Boost_Build_Cleanup
        BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost-build/b2
        DEPENDS
            Boost_Build
        COMMAND
            mv
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost/tools/build/src/engine/b2 ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost-build/b2
        COMMAND
            rm ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost/tools/build/src/engine/bjam
    )
    add_custom_target(Boost
        DEPENDS
            Boost_Build_Cleanup
        BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_log.a
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_log_setup.a
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_thread.a
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_filesystem.a
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_program_options.a
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_iostreams.a
        COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost-build/b2 --prefix=${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install --with-log --with-filesystem --with-program_options --with-iostreams --stagedir=${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/stage link=static threading=multi
        COMMAND
            ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost-build/b2 --prefix=${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install --with-log --with-filesystem --with-program_options --with-iostreams --stagedir=${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/stage link=static threading=multi install
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost
    )
    include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/include)

    add_library(boost_log STATIC IMPORTED)
    add_library(boost_log_setup STATIC IMPORTED)
    set_property(TARGET boost_log_setup PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_log_setup.a)
    add_library(boost_thread STATIC IMPORTED)
    set_property(TARGET boost_thread PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_thread.a)
    target_link_libraries(boost_log INTERFACE boost_log_setup boost_thread -lpthread)

    add_library(Boost::log ALIAS boost_log)
    set_property(TARGET boost_log PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_log.a)
    add_dependencies(boost_log Boost)

    add_library(boost_filesystem STATIC IMPORTED)
    add_library(Boost::filesystem ALIAS boost_filesystem)
    set_property(TARGET boost_filesystem PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_filesystem.a)
    add_dependencies(boost_filesystem Boost)

    add_library(boost_program_options STATIC IMPORTED)
    add_library(Boost::program_options ALIAS boost_program_options)
    set_property(TARGET boost_program_options PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_program_options.a)
    add_dependencies(boost_program_options Boost)

    add_library(boost_iostreams STATIC IMPORTED)
    add_library(Boost::iostreams ALIAS boost_iostreams)
    set_property(TARGET boost_iostreams PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost/install/lib/libboost_iostreams.a)
    target_link_libraries(boost_iostreams INTERFACE PkgConfig::zstd)
    add_dependencies(boost_iostreams Boost)
endif()

CHECK_INCLUDE_FILE_CXX("toml++/toml.h" HAVE_TOMLPLUSPLUS)
if(NOT HAVE_TOMLPLUSPLUS)
    message(STATUS "system tomlplusplus not found, using bundled version")
    include_directories(SYSTEM "third_party/tomlplusplus/include")
endif()

execute_process(
    COMMAND bash -c "LANG=C ${CMAKE_CXX_COMPILER} -v 2> >(grep Target | cut -d ' ' -f 2 | xargs echo)"
    OUTPUT_VARIABLE DISTPLUSPLUS_BUILD_TARGET)
string(STRIP ${DISTPLUSPLUS_BUILD_TARGET} DISTPLUSPLUS_BUILD_TARGET)
if(NOT DISTPLUSPLUS_BUILD_TARGET)
    message(FATAL_ERROR "could not determine build target from $CXX -v")
endif()
add_compile_definitions("DISTPLUSPLUS_BUILD_TARGET=${DISTPLUSPLUS_BUILD_TARGET}")

include_directories("src")
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/protos)
add_subdirectory("protos")
add_subdirectory("src/common")
add_subdirectory("src/client")
add_subdirectory("src/server")

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
    # I'm not exactly happy with this
    list(FILTER ${targets} EXCLUDE REGEX test)
    list(FILTER ${targets} EXCLUDE REGEX coverage)
    list(FILTER ${targets} EXCLUDE REGEX install)
endmacro()

get_all_targets_recursive(src_targets src/common)
get_all_targets_recursive(src_targets src/client)
get_all_targets_recursive(src_targets src/server)
get_all_targets_recursive(src_targets protos)

if(BUILD_TESTING)
    add_subdirectory("test")
    get_all_targets_recursive(src_targets test)
endif()

foreach(target ${src_targets})
    if(NOT SYSTEM_BOOST)
        add_dependencies(${target} Boost)
    endif()
    if(NOT SYSTEM_GRPC)
        add_dependencies(${target} grpc-third_party)
    endif()
    add_dependencies(${target} gen-protos)
endforeach()
