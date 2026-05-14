cmake_minimum_required(VERSION 3.10)
project(rpk-superbuild LANGUAGES C CXX)

option(WITH_RAYTRACING "Enable raytracing modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

get_filename_component(RPK_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
set(RPK_BASE_BINARY_DIR "${RPK_ROOT_DIR}/base/build" CACHE PATH "Binary directory for the vitral library build")
set(RPK_OPENGL_BINARY_DIR "${RPK_ROOT_DIR}/opengl/build" CACHE PATH "Binary directory for the vitral-opengl library build")
set(RPK_RENDERPARK_BINARY_DIR "${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build" CACHE PATH "Binary directory for the Renderpark application build")

add_subdirectory(
    ${RPK_ROOT_DIR}/base
    ${RPK_BASE_BINARY_DIR}
)

add_subdirectory(
    ${RPK_ROOT_DIR}/opengl
    ${RPK_OPENGL_BINARY_DIR}
)

add_subdirectory(
    ${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication
    ${RPK_RENDERPARK_BINARY_DIR}
)
