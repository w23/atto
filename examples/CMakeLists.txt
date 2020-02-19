cmake_minimum_required(VERSION 3.10)

if(WIN32)
	# required to set SUBSYSTEM linker flag to WINDOWS
	# because we don't want it to be CONSOLE
	set(ATTO_EXE_SUBSYSTEM WIN32)
endif()

function(add_example EXAMPLE_NAME)
	project(${EXAMPLE_NAME})
	add_executable(${EXAMPLE_NAME} ${ATTO_EXE_SUBSYSTEM}
		${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}.c)
	target_link_libraries(${EXAMPLE_NAME} atto)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES
		C_STANDARD 99
		C_STANDARD_REQUIRED TRUE
		C_EXTENSIONS OFF)
endfunction()

add_example(app)
add_example(cube)
add_example(fb)
add_example(tri)
add_example(tribench)
