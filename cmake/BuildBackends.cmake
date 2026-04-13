cmake_minimum_required(VERSION 3.15)

get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED BUILD_CONFIG)
	set(BUILD_CONFIG "Release")
endif()

if(NOT DEFINED GENERATOR)
	set(GENERATOR "")
endif()

if(NOT DEFINED ARCHITECTURE)
	set(ARCHITECTURE "")
endif()

if(NOT DEFINED TOOLSET)
	set(TOOLSET "")
endif()

if(NOT DEFINED PARALLEL_LEVEL)
	set(PARALLEL_LEVEL "")
endif()

function(run_checked)
	string(JOIN " " pretty_command ${ARGN})
	message(STATUS "Running: ${pretty_command}")
	execute_process(
		COMMAND ${ARGN}
		RESULT_VARIABLE command_result
	)
	if(NOT command_result EQUAL 0)
		message(FATAL_ERROR "Command failed with exit code ${command_result}")
	endif()
endfunction()

function(configure_and_build_backend backend)
	string(TOLOWER "${backend}" backend_dir)
	set(build_dir "${SOURCE_DIR}/build-${backend_dir}")

	set(configure_command
		"${CMAKE_COMMAND}"
		"-S" "${SOURCE_DIR}"
		"-B" "${build_dir}"
		"-DBACKEND_RENDERER=${backend}"
		"-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}"
	)

	if(NOT GENERATOR STREQUAL "")
		list(APPEND configure_command "-G" "${GENERATOR}")
	endif()

	if(NOT ARCHITECTURE STREQUAL "")
		list(APPEND configure_command "-A" "${ARCHITECTURE}")
	endif()

	if(NOT TOOLSET STREQUAL "")
		list(APPEND configure_command "-T" "${TOOLSET}")
	endif()

	run_checked(${configure_command})

	set(build_command
		"${CMAKE_COMMAND}"
		"--build" "${build_dir}"
		"--config" "${BUILD_CONFIG}"
	)

	if(NOT PARALLEL_LEVEL STREQUAL "")
		list(APPEND build_command "--parallel" "${PARALLEL_LEVEL}")
	endif()

	run_checked(${build_command})
endfunction()

message(STATUS "Source directory: ${SOURCE_DIR}")
message(STATUS "Build configuration: ${BUILD_CONFIG}")

configure_and_build_backend("OpenGL2")

if(CMAKE_HOST_WIN32)
	configure_and_build_backend("D3D11")
else()
	message(STATUS "Skipping D3D11 build because that backend is only supported on Windows hosts.")
endif()

message(STATUS "Finished building backends. Artifacts are written under ${SOURCE_DIR}/bin/<backend>/")