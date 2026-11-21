cmake_minimum_required(VERSION 3.10)
project(rpk-superbuild LANGUAGES C CXX)

option(WITH_RAYTRACING "Enable raytracing modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)
option(WITH_RAY_CAPTURE "Count visibility queries by path and write the ray corpus (step 1 capture build)" OFF)
option(WITH_RAY_REPLAY "Build the ray corpus replay of plan step 4 (testsuite/ApplicationCases/RayCorpusReplay)" ON)

get_filename_component(RPK_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
set(RPK_BASE_BINARY_DIR "${RPK_ROOT_DIR}/base/build" CACHE PATH "Binary directory for the vitral library build")
set(RPK_OPENGL_BINARY_DIR "${RPK_ROOT_DIR}/opengl/build" CACHE PATH "Binary directory for the vitral-opengl library build")
set(RPK_RENDERPARK_BINARY_DIR "${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build" CACHE PATH "Binary directory for the Renderpark application build")
set(RPK_RAY_REPLAY_BINARY_DIR "${RPK_ROOT_DIR}/testsuite/ApplicationCases/RayCorpusReplay/build" CACHE PATH "Binary directory for the ray corpus replay build")

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

if(WITH_RAY_REPLAY)
    add_subdirectory(
        ${RPK_ROOT_DIR}/testsuite/ApplicationCases/RayCorpusReplay
        ${RPK_RAY_REPLAY_BINARY_DIR}
    )
endif()

# T0 duplicate-class audit: two definitions of one class name in the link
# closure fail the build (scripts/classAudit.py; known debt in classAudit.allow)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    if(WITH_RAYTRACING)
        set(RPK_CLASS_AUDIT_RAYTRACING on)
    else()
        set(RPK_CLASS_AUDIT_RAYTRACING off)
    endif()
    add_custom_target(classAudit ALL
        COMMAND ${Python3_EXECUTABLE} ${RPK_ROOT_DIR}/scripts/classAudit.py --raytracing ${RPK_CLASS_AUDIT_RAYTRACING}
        COMMENT "Duplicate-class audit of the link closure"
        VERBATIM)
else()
    message(WARNING "Python 3 not found: the duplicate-class audit (scripts/classAudit.py) will not run")
endif()
