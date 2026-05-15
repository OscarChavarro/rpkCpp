cmake_minimum_required(VERSION 3.10)
project(rpk-superbuild LANGUAGES C CXX)

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)
option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)

get_filename_component(RPK_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
set(RPK_BASE_BINARY_DIR "${RPK_ROOT_DIR}/base/build" CACHE PATH "Binary directory for the turboc base library build")
set(RPK_RENDERPARK_BINARY_DIR "${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build" CACHE PATH "Binary directory for the Renderpark application build")

add_subdirectory(
    ${RPK_ROOT_DIR}/base
    ${RPK_BASE_BINARY_DIR}
)

add_subdirectory(
    ${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication
    ${RPK_RENDERPARK_BINARY_DIR}
)
