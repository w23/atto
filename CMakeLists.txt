cmake_minimum_required(VERSION 3.10)
project(atto C)

set(OpenGL_GL_PREFERENCE GLVND)

if(WIN32)
	set(ATTO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/app_windows.c)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
	set(ATTO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/app_linux.c)
	set(ATTO_PRIVATE_LIBS m)

	option(ATTO_APP_KMS "Use no-desktop raw libdrm KMS" OFF)
	option(ATTO_APP_EGL "Use EGL for OpenGL context creation" ON)

	if (ATTO_APP_KMS)
		# KMS implies EGL
		set(ATTO_APP_EGL YES)

		# TODO OpenGL flavor selection
		set(ATTO_APP_GLES YES)

		set(ATTO_DEFS ${ATTO_DEFS} ATTO_KMS)
		set(ATTO_SOURCES ${ATTO_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/app_main.c)

		find_package(PkgConfig REQUIRED)
		pkg_check_modules(libdrm REQUIRED IMPORTED_TARGET libdrm)
		pkg_check_modules(gbm REQUIRED IMPORTED_TARGET gbm)
		set(ATTO_PRIVATE_LIBS ${ATTO_PRIVATE_LIBS} PkgConfig::libdrm PkgConfig::gbm)
	else()
		find_package(X11 REQUIRED)
		set(ATTO_PRIVATE_LIBS
			${ATTO_PRIVATE_LIBS}
			${X11_LIBRARIES}
			${X11_Xext_LIB}
			${X11_Xfixes_LIB}
			${X11_Xrandr_LIB}
			)
		set(ATTO_SOURCES ${ATTO_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/app_x11.c)
	endif()

else()
	message(FATAL_ERROR "Not ported")
endif()

add_library(atto STATIC ${ATTO_SOURCES})

if (ATTO_APP_GLES)
	set(ATTO_DEFS ${ATTO_DEFS} ATTO_GLES)
	set(OPENGL_USE_GLES2 YES)
endif()

if (ATTO_APP_EGL)
	target_compile_definitions(atto PUBLIC ${ATTO_DEFS} ATTO_EGL)

	set(OPENGL_USE_EGL YES)
	find_package(OpenGL REQUIRED EGL)
	set(ATTO_PUBLIC_LIBS ${ATTO_PUBLIC_LIBS} OpenGL::GL)
	set(ATTO_PRIVATE_LIBS ${ATTO_PRIVATE_LIBS} OpenGL::EGL)
else()
	find_package(OpenGL REQUIRED)
	set(ATTO_PUBLIC_LIBS OpenGL::GL)
endif()

target_include_directories(atto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(atto
	PUBLIC ${ATTO_PUBLIC_LIBS}
	PRIVATE ${ATTO_PRIVATE_LIBS})

set_target_properties(atto PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED TRUE
	C_EXTENSIONS TRUE)

message("C compiler ID: " ${CMAKE_C_COMPILER_ID})
target_compile_options(atto PRIVATE
	$<$<C_COMPILER_ID:MSVC>:/W4 /WX>
	$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
	$<$<C_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>
)

# Also build examples if current cmake is root (not included from other cmake)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	add_subdirectory(examples)
endif()
