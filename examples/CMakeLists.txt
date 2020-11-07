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
	target_link_libraries(${EXAMPLE_NAME} atto m)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES
		C_STANDARD 99
		C_STANDARD_REQUIRED TRUE
		C_EXTENSIONS OFF)
endfunction()

# add_example(app)
# add_example(cube)
# add_example(fb)
# add_example(tri)
# add_example(tribench)
add_example(vk_tri)

function(add_shaders)
	cmake_parse_arguments(PARSE_ARGV 0 SHADERS "" "TARGET" "FILES")
	foreach(SHADER ${SHADERS_FILES})
		add_custom_command(
			OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${SHADER}.spv
			COMMAND glslc
			${CMAKE_SOURCE_DIR}/examples/${SHADER} -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${SHADER}.spv
			MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/examples/${SHADER}
			COMMENT "Building ${SHADER}"
		)
		list(APPEND SHADERS_DEPS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${SHADER}.spv")
	endforeach()
	add_custom_target(${SHADERS_TARGET} ALL DEPENDS ${SHADERS_DEPS})
endfunction()

add_shaders(TARGET vk_tri_shaders FILES shader.frag shader.vert)
