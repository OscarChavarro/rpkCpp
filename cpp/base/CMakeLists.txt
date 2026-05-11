cmake_minimum_required(VERSION 3.10)
project(vitral LANGUAGES C CXX)

option(WITH_RAYTRACING "Enable raytracing modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

if(WITH_OPENGL)
    if(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(GLUT_FRAMEWORK GLUT)
        if(NOT OPENGL_FRAMEWORK OR NOT GLUT_FRAMEWORK)
            message(WARNING "OpenGL support requested, but OpenGL/GLUT frameworks were not found. Building without OpenGL support.")
            set(WITH_OPENGL OFF)
        endif()
    else()
        find_library(RPK_OPENGL_GL_LIBRARY GL)
        find_library(RPK_OPENGL_GLU_LIBRARY GLU)
        find_library(RPK_OPENGL_GLUT_LIBRARY glut)
        if(NOT RPK_OPENGL_GL_LIBRARY OR NOT RPK_OPENGL_GLU_LIBRARY OR NOT RPK_OPENGL_GLUT_LIBRARY)
            message(WARNING "OpenGL support requested, but GL/GLU/GLUT libraries were not found. Building without OpenGL support.")
            set(WITH_OPENGL OFF)
        endif()
    endif()
endif()

add_compile_options(-ffast-math -O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE VITRAL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vsdk/toolkit/*.cpp
)

if(NOT WITH_OPENGL)
    list(FILTER VITRAL_SRC EXCLUDE REGEX "/render/opengl/")
endif()

if(NOT WITH_MGF)
    list(FILTER VITRAL_SRC EXCLUDE REGEX "/io/mgf/")
endif()

if(NOT WITH_RAYTRACING)
    list(FILTER VITRAL_SRC EXCLUDE REGEX "/raycasting/")
endif()

add_library(vitral SHARED ${VITRAL_SRC})

target_include_directories(vitral
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(vitral PROPERTIES
    OUTPUT_NAME vitral
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WITH_RAYTRACING)
    target_compile_definitions(vitral PUBLIC RAYTRACING_ENABLED=true)
endif()

if(WITH_OPENGL)
    target_compile_definitions(vitral PUBLIC OPEN_GL_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(vitral PUBLIC MGF_ENABLED=true)
endif()

if(WITH_OPENGL)
    if(APPLE)
        message(STATUS "Using native macOS frameworks: ${OPENGL_FRAMEWORK}, ${GLUT_FRAMEWORK}")
        target_compile_definitions(vitral PRIVATE GL_SILENCE_DEPRECATION)
        target_link_libraries(vitral PUBLIC ${OPENGL_FRAMEWORK} ${GLUT_FRAMEWORK})
    else()
        target_link_libraries(vitral PUBLIC ${RPK_OPENGL_GLU_LIBRARY} ${RPK_OPENGL_GL_LIBRARY} ${RPK_OPENGL_GLUT_LIBRARY})
    endif()
endif()
