cmake_minimum_required(VERSION 3.10)
project(rpk-superbuild LANGUAGES C CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

option(WITH_RAYTRACING "Enable raytracing modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

get_filename_component(RPK_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
set(RPK_VITRAL_BINARY_DIR "${RPK_ROOT_DIR}/base/build" CACHE PATH "Binary directory for the vitral library build (base/, the VITRAL copy)")
set(RPK_BASE_BINARY_DIR "${RPK_ROOT_DIR}/rpkBase/build" CACHE PATH "Binary directory for the rpkBase library build")
set(RPK_OPENGL1_BINARY_DIR "${RPK_ROOT_DIR}/rpkOpenGL1/build" CACHE PATH "Binary directory for the rpkOpenGL1 library build")
set(RPK_RENDERPARK_BINARY_DIR "${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build" CACHE PATH "Binary directory for the Renderpark application build")

# Layers: base/ (vitral, the byte-identical VITRAL subset) <- rpkBase/
# (RenderPark's radiosity, raytracing and scene classes) <- rpkOpenGL1/
# (OpenGL 1 viewer, only WITH_OPENGL) <- the RenderPark application
add_subdirectory(
    ${RPK_ROOT_DIR}/base
    ${RPK_VITRAL_BINARY_DIR}
)

add_subdirectory(
    ${RPK_ROOT_DIR}/rpkBase
    ${RPK_BASE_BINARY_DIR}
)

if(WITH_OPENGL)
    add_subdirectory(
        ${RPK_ROOT_DIR}/rpkOpenGL1
        ${RPK_OPENGL1_BINARY_DIR}
    )
endif()

add_subdirectory(
    ${RPK_ROOT_DIR}/testsuite/ApplicationCases/RenderparkApplication
    ${RPK_RENDERPARK_BINARY_DIR}
)

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
