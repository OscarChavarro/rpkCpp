cmake_minimum_required(VERSION 3.10)
project(RenderparkApplicationTurboc LANGUAGES C CXX)

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)
option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-Wall -pedantic)
endif()

add_compile_options(-ffast-math -O3)

if((UNIX OR APPLE) AND FORCE_32BIT_UNIX_APPLE)
    # -mfpmath=sse keeps 32-bit float results consistent with the 64-bit cpp
    # reference build (the x87 default computes intermediates at 80 bits)
    add_compile_options(-m32 -msse2 -mfpmath=sse)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

get_filename_component(VITRAL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../base REALPATH)
set(VITRAL_INCLUDE_DIR "${VITRAL_BASE_DIR}/src" CACHE PATH "Include directory for the turboc vitral library")
set(VITRAL_LIBRARY_PATH "${VITRAL_BASE_DIR}/build/libvitral-turboc.so" CACHE FILEPATH "Shared library path for turboc vitral when building standalone")

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

if(TARGET vitral-turboc)
    target_link_libraries(rpk PRIVATE vitral-turboc)
else()
    add_library(vitral-turboc SHARED IMPORTED)
    set_target_properties(vitral-turboc PROPERTIES
        IMPORTED_LOCATION ${VITRAL_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${VITRAL_INCLUDE_DIR}
    )
    target_link_libraries(rpk PRIVATE vitral-turboc)
endif()
