cmake_minimum_required(VERSION 3.10)
project(vitral-opengl LANGUAGES C CXX)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-ffast-math -O3)

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

file(GLOB_RECURSE VITRAL_OPENGL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/render/opengl/*.cpp
)

add_library(vitral-opengl SHARED ${VITRAL_OPENGL_SRC})

target_include_directories(vitral-opengl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
        ${CMAKE_CURRENT_SOURCE_DIR}/../base/src/main
)

target_link_libraries(vitral-opengl
    PUBLIC
        vitral
)

target_compile_definitions(vitral-opengl PUBLIC OPEN_GL_ENABLED=true)

set_target_properties(vitral-opengl PROPERTIES
    OUTPUT_NAME vitral-opengl
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(APPLE)
    message(STATUS "Using native macOS frameworks for vitral-opengl: ${OPENGL_FRAMEWORK}, ${GLUT_FRAMEWORK}")
    target_compile_definitions(vitral-opengl PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(vitral-opengl PUBLIC ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
else()
    target_link_libraries(vitral-opengl PUBLIC ${RPK_OPENGL_GLU_LIBRARY} ${RPK_OPENGL_GL_LIBRARY} ${RPK_OPENGL_GLUT_LIBRARY})
endif()
