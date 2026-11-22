cmake_minimum_required(VERSION 3.10)
project(rpkOpenGL1 LANGUAGES C CXX)

# RenderPark's interactive viewer and visual debug tools over fixed-function
# OpenGL 1 (glBegin/glMatrixMode) and GLUT. No VITRAL file is taken here: the
# VITRAL OpenGL 1 renderers (VITRAL cpp/opengl1) are not used by RenderPark.

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(GLUT_FRAMEWORK GLUT REQUIRED)
else()
    find_library(RPK_OPENGL_GL_LIBRARY GL REQUIRED)
    find_library(RPK_OPENGL_GLU_LIBRARY GLU REQUIRED)
    find_library(RPK_OPENGL_GLUT_LIBRARY glut REQUIRED)
endif()

file(GLOB_RECURSE RPK_OPENGL1_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp
)

add_library(rpkOpenGL1 SHARED ${RPK_OPENGL1_SRC})

target_include_directories(rpkOpenGL1
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

target_link_libraries(rpkOpenGL1
    PUBLIC
        rpkBase
)

target_compile_definitions(rpkOpenGL1 PUBLIC OPEN_GL_ENABLED=true)

set_target_properties(rpkOpenGL1 PROPERTIES
    OUTPUT_NAME rpkOpenGL1
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(APPLE)
    message(STATUS "Using native macOS frameworks for rpkOpenGL1: ${OPENGL_FRAMEWORK}, ${GLUT_FRAMEWORK}")
    target_compile_definitions(rpkOpenGL1 PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(rpkOpenGL1 PUBLIC ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
else()
    target_link_libraries(rpkOpenGL1 PUBLIC ${RPK_OPENGL_GLU_LIBRARY} ${RPK_OPENGL_GL_LIBRARY} ${RPK_OPENGL_GLUT_LIBRARY})
endif()
