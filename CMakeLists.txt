cmake_minimum_required(VERSION 3.10)
project(atto C)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

if(WIN32)
	set(ATTO_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/src/app_windows.c)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
	# find_package(X11 REQUIRED)
	# set(ATTO_LIBS
	# 	${X11_LIBRARIES}
	# 	${X11_Xext_LIB}
	# 	${X11_Xfixes_LIB}
	# 	m)
	# set(ATTO_SRC
	# 	${CMAKE_CURRENT_SOURCE_DIR}/src/app_linux.c
	# 	# ${CMAKE_CURRENT_SOURCE_DIR}/src/app_x11.c)
	# 	${CMAKE_CURRENT_SOURCE_DIR}/src/app_wayland.c)
	set(ATTO_SRC
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/app_linux.c
	# 	# ${CMAKE_CURRENT_SOURCE_DIR}/src/app_x11.c)
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/app_wayland.c
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/app_vulkan.c
	 	${CMAKE_CURRENT_SOURCE_DIR}/3p/wl-xdg-proto.c
	)
else()
	message(FATAL_ERROR "Not ported")
endif()

add_library(atto STATIC ${ATTO_SRC})
target_include_directories(atto PUBLIC include)
target_include_directories(atto PRIVATE 3p)
target_link_libraries(atto
	#PUBLIC OpenGL::GL
	PUBLIC wayland-client vulkan
	PRIVATE ${ATTO_LIBS})
set_target_properties(atto PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED TRUE
	C_EXTENSIONS ON)

# check if current cmake is not included into other cmake
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	add_subdirectory(examples)
endif()
