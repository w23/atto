cmake_minimum_required(VERSION 3.10)
project(atto C)

set(OpenGL_GL_PREFERENCE GLVND)
#set(OpenGL_GL_PREFERENCE LEGACY)

if(WIN32)
	set(ATTO_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/app_windows.c)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
	option(ATTO_USE_EGL "Use EGL for OpenGL context creation" ON)
	option(ATTO_USE_KMS "Use no-desktop raw libdrm KMS" OFF)

	set(ATTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/app_linux.c)
	set(ATTO_LIBS m)

	if (ATTO_USE_KMS)
		set(ATTO_USE_EGL YES)

		find_package(PkgConfig REQUIRED)
		pkg_check_modules(libdrm REQUIRED IMPORTED_TARGET libdrm)
		pkg_check_modules(gbm REQUIRED IMPORTED_TARGET gbm)
		set(ATTO_LIBS ${ATTO_LIBS} PkgConfig::libdrm PkgConfig::gbm)

		set(ATTO_SRC ${ATTO_SRC}
			#${CMAKE_CURRENT_SOURCE_DIR}/src/app_kms.c
			${CMAKE_CURRENT_SOURCE_DIR}/src/app_main.c
			)

		set(ATTO_DEFS ${ATTO_DEFS} ATTO_KMS)

	else()
		find_package(X11 REQUIRED)
		set(ATTO_LIBS
			${ATTO_LIBS}
			${X11_LIBRARIES}
			${X11_Xext_LIB}
			${X11_Xfixes_LIB}
			)
		set(ATTO_SRC ${ATTO_SRC}
			${CMAKE_CURRENT_SOURCE_DIR}/src/app_x11.c)
	endif()

else()
	message(FATAL_ERROR "Not ported")
endif()

add_library(atto STATIC ${ATTO_SRC})

if (ATTO_USE_EGL)
	#needed to avoid OPENGL_opengl_LIBRARY find errors ... FingOpenGL.cmake is weird
	# FIXME TEMP
	set(OPENGL_USE_EGL YES)
	# FIXME TEMP
	set(OPENGL_USE_GLES2 YES)

	find_package(OpenGL REQUIRED EGL)
	set(ATTO_GL_LIBS OpenGL::GL OpenGL::EGL)
	target_compile_definitions(atto PUBLIC ${ATTO_DEFS} ATTO_EGL)
else()
	find_package(OpenGL REQUIRED)
	set(ATTO_GL_LIBS OpenGL::GL)
endif()

target_include_directories(atto PUBLIC include)

target_link_libraries(atto
	PUBLIC ${ATTO_GL_LIBS}
	PRIVATE ${ATTO_LIBS})

set_target_properties(atto PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED TRUE
	C_EXTENSIONS ON)

# check if current cmake is not included into other cmake
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	add_subdirectory(examples)
endif()
