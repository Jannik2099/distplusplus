add_executable(ServerMsgHelper "ServerMsgHelper.cpp")
target_link_libraries(ServerMsgHelper distplusplus::protobuf distplusplus::grpc distplusplus::common)

function(add_simple_test name libraries)
	set(filename "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp")
	if(NOT EXISTS ${filename})
		message(FATAL_ERROR "unit test source ${filename} does not exist")
	endif()
	add_executable("${name}" "${filename}")
	target_link_libraries("${name}" ${libraries})
	add_test(NAME "${name}" COMMAND "${name}")
	if(COVERAGE)
		set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT LLVM_PROFILE_FILE=${name}.profraw)
		set_property(TEST "${name}" PROPERTY ADDITIONAL_CLEAN_FILES "$<TARGET_FILE:${name}>.profraw")
	endif()
endfunction()

function(add_bash_test name)
	set(filename "${CMAKE_CURRENT_SOURCE_DIR}/${name}.sh")
	if(NOT EXISTS ${filename})
		message(FATAL_ERROR "unit test script ${filename} does not exist")
	endif()
	execute_process(
		COMMAND "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/BashTestVerify.sh" ${filename}
		RESULT_VARIABLE ret
		OUTPUT_QUIET ERROR_QUIET
	)
	if(NOT ret EQUAL 0)
		message(FATAL_ERROR "unit test script ${filename} does not define a runTest function")
	endif()

	add_test(NAME "${name}" COMMAND "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/BashTestLib.sh" ${filename} ${name} $<TARGET_FILE:distplusplus> $<TARGET_FILE:distplusplus-server> $<TARGET_FILE:ServerMsgHelper>)
	if(COVERAGE)
		set_property(TEST "${name}" APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${name}-client.profraw")
		set_property(TEST "${name}" APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${name}-server.profraw")
	endif()
endfunction()

include_directories(".")
add_subdirectory("common")
add_subdirectory("client")
add_subdirectory("server")
if(COVERAGE)
	set(LLVM_PROFDATA "llvm-profdata" CACHE STRING "path to llvm-profdata")
	set(LLVM_COV "llvm-cov" CACHE STRING "path to llvm-cov")
	add_custom_target(coverage COMMAND env LLVM_PROFDATA=${LLVM_PROFDATA} LLVM_COV=${LLVM_COV} ${CMAKE_CURRENT_SOURCE_DIR}/create_cov.sh WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
	set_property(TARGET coverage PROPERTY ADDITIONAL_CLEAN_FILES
		"${PROJECT_BINARY_DIR}/coverage.txt,${PROJECT_BINARY_DIR}/coverage.html,${PROJECT_BINARY_DIR}/coverage-lcov.txt,${PROJECT_BINARY_DIR}/coverage.profdata")
endif()
