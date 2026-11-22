cmake_minimum_required(VERSION 3.10)
project(RenderparkApplicationTurboc LANGUAGES C CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)
option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-Wall -pedantic)
endif()

add_compile_options(-O3)

if((UNIX OR APPLE) AND FORCE_32BIT_UNIX_APPLE)
    # -mfpmath=sse keeps 32-bit float results consistent with the 64-bit cpp
    # reference build (the x87 default computes intermediates at 80 bits)
    add_compile_options(-m32 -msse2 -mfpmath=sse)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

get_filename_component(RPK_TURBOC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../.. REALPATH)
set(VITRAL_INCLUDE_DIR "${RPK_TURBOC_DIR}/base/src/main" CACHE PATH "Include directory for the turboc vitral library")
set(VITRAL_LIBRARY_PATH "${RPK_TURBOC_DIR}/base/build/libvitral-turboc.so" CACHE FILEPATH "Shared library path for turboc vitral when building standalone")
set(RPK_BASE_INCLUDE_DIR "${RPK_TURBOC_DIR}/rpkBase/src/main" CACHE PATH "Include directory for the turboc rpkBase library")
set(RPK_BASE_LIBRARY_PATH "${RPK_TURBOC_DIR}/rpkBase/build/librpkBase-turboc.so" CACHE FILEPATH "Shared library path for turboc rpkBase when building standalone")

file(GLOB_RECURSE RPK_APP_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(NOT RAYTRACING_ENABLED)
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/Raytrace\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupRaytracing\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupRaytracingMethod\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupStochasticRelaxationRadiosity\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupRandomWalkRadiosity\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupStochasticRaytracing\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupBidirectionalRaytracing\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupPhotonMap\\.cpp$")
    list(FILTER RPK_APP_SRC EXCLUDE REGEX "/OptionsGroupRayMatter\\.cpp$")
endif()

add_executable(rpk ${RPK_APP_SRC})

target_include_directories(rpk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${RPK_BASE_INCLUDE_DIR}
        ${VITRAL_INCLUDE_DIR}
)

set_target_properties(rpk PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(RAYTRACING_ENABLED)
    target_compile_definitions(rpk PRIVATE RAYTRACING_ENABLED)
endif()

if(MGF_ENABLED)
    target_compile_definitions(rpk PRIVATE MGF_ENABLED)
endif()

if(NOT TARGET vitral-turboc)
    add_library(vitral-turboc SHARED IMPORTED)
    set_target_properties(vitral-turboc PROPERTIES
        IMPORTED_LOCATION ${VITRAL_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${VITRAL_INCLUDE_DIR}
    )
endif()

if(NOT TARGET rpkBase-turboc)
    add_library(rpkBase-turboc SHARED IMPORTED)
    set_target_properties(rpkBase-turboc PROPERTIES
        IMPORTED_LOCATION ${RPK_BASE_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES "${RPK_BASE_INCLUDE_DIR};${VITRAL_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES vitral-turboc
    )
endif()

target_link_libraries(rpk PRIVATE rpkBase-turboc vitral-turboc)
