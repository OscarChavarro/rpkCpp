cmake_minimum_required(VERSION 3.10)
project(RenderparkApplication LANGUAGES C CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

option(WITH_RAYCASTING "Enable raycasting modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(RPK_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../.. REALPATH)
set(VITRAL_INCLUDE_DIR "${RPK_CPP_DIR}/base/src/main" CACHE PATH "Include directory for the vitral library")
set(VITRAL_LIBRARY_PATH "${RPK_CPP_DIR}/base/build/libvitral.so" CACHE FILEPATH "Shared library path for vitral when building standalone")
set(RPK_BASE_INCLUDE_DIR "${RPK_CPP_DIR}/rpkBase/src/main" CACHE PATH "Include directory for the rpkBase library")
set(RPK_BASE_LIBRARY_PATH "${RPK_CPP_DIR}/rpkBase/build/librpkBase.so" CACHE FILEPATH "Shared library path for rpkBase when building standalone")
set(RPK_OPENGL1_INCLUDE_DIR "${RPK_CPP_DIR}/rpkOpenGL1/src/main" CACHE PATH "Include directory for the rpkOpenGL1 library")
set(RPK_OPENGL1_LIBRARY_PATH "${RPK_CPP_DIR}/rpkOpenGL1/build/librpkOpenGL1.so" CACHE FILEPATH "Shared library path for rpkOpenGL1 when building standalone")

file(GLOB_RECURSE RPK_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(NOT WITH_RAYCASTING)
    list(FILTER RPK_SRC EXCLUDE REGEX "/Raytrace\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupRaytracing\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupRaytracingMethod\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupStochasticRelaxationRadiosity\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupRandomWalkRadiosity\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupStochasticRaytracing\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupBidirectionalRaytracing\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupPhotonMap\\.cpp$")
    list(FILTER RPK_SRC EXCLUDE REGEX "/OptionsGroupRayMatter\\.cpp$")
endif()

add_executable(rpk ${RPK_SRC})

target_include_directories(rpk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${RPK_BASE_INCLUDE_DIR}
        ${VITRAL_INCLUDE_DIR}
)

set_target_properties(rpk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WITH_RAYCASTING)
    target_compile_definitions(rpk PRIVATE RAYTRACING_ENABLED=true)
endif()

if(WITH_OPENGL)
    target_compile_definitions(rpk PRIVATE OPEN_GL_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(rpk PRIVATE MGF_ENABLED=true)
endif()

if(NOT TARGET vitral)
    add_library(vitral SHARED IMPORTED)
    set_target_properties(vitral PROPERTIES
        IMPORTED_LOCATION ${VITRAL_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${VITRAL_INCLUDE_DIR}
    )
endif()

if(NOT TARGET rpkBase)
    add_library(rpkBase SHARED IMPORTED)
    set_target_properties(rpkBase PROPERTIES
        IMPORTED_LOCATION ${RPK_BASE_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES "${RPK_BASE_INCLUDE_DIR};${VITRAL_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES vitral
    )
endif()

target_link_libraries(rpk PRIVATE rpkBase vitral)

if(WITH_OPENGL)
    if(NOT TARGET rpkOpenGL1)
        add_library(rpkOpenGL1 SHARED IMPORTED)
        set_target_properties(rpkOpenGL1 PROPERTIES
            IMPORTED_LOCATION ${RPK_OPENGL1_LIBRARY_PATH}
            INTERFACE_INCLUDE_DIRECTORIES "${RPK_OPENGL1_INCLUDE_DIR};${RPK_BASE_INCLUDE_DIR};${VITRAL_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES rpkBase
        )
    endif()
    target_link_libraries(rpk PRIVATE rpkOpenGL1)
endif()
