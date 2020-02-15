cmake_minimum_required(VERSION 3.10)

############## atto lib ##############

project(atto C)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

if(WIN32)
  set(ATTO_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app_windows.c)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
  set(ATTO_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app_linux.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app_x11.c)
  set(ATTO_LIBS X11 Xfixes m)
else()
  message(FATAL_ERROR "Not ported")
endif()

add_library(atto STATIC ${ATTO_SRC})
target_include_directories(atto PUBLIC include)
target_link_libraries(atto OpenGL::GL ${ATTO_LIBS})
set_target_properties(atto PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED TRUE
  C_EXTENSIONS ON)

############## examples ##############

function(add_example EXAMPLE_NAME)
  project(${EXAMPLE_NAME})
  if(WIN32)
    set(ATTO_EXE_SYSTEM WIN32)
  endif()
  add_executable(${EXAMPLE_NAME} ${ATTO_EXE_SYSTEM}
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/${EXAMPLE_NAME}.c)
  target_link_libraries(${EXAMPLE_NAME} atto)
  set_target_properties(${EXAMPLE_NAME} PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS ON)
endfunction()

add_example(app)
add_example(cube)
add_example(fb)
add_example(tri)
add_example(tribench)