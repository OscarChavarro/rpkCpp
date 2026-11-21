cmake_minimum_required(VERSION 3.10)
project(RayCorpusReplayTurboc LANGUAGES C CXX)

# Plan step 4, item 4: replays a step 1 ray corpus through the legacy voxel
# grid and VITRAL's spatial indexes over the scene the rpk command line builds.
# It compiles the RenderPark application sources (except its main) with the
# same options, so the scene and the face test are the application's own.
# The corpus is read sequentially with 64-bit file offsets
# (_FILE_OFFSET_BITS=64), so that the 32-bit build opens files beyond 2 GB.

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

get_filename_component(RPK_APPLICATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../RenderparkApplication REALPATH)
get_filename_component(VITRAL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../base REALPATH)
set(VITRAL_INCLUDE_DIR "${VITRAL_BASE_DIR}/src/main" CACHE PATH "Include directory for the turboc vitral library")
set(VITRAL_LIBRARY_PATH "${VITRAL_BASE_DIR}/build/libvitral-turboc.so" CACHE FILEPATH "Shared library path for turboc vitral when building standalone")

file(GLOB_RECURSE RPK_APPLICATION_SRC CONFIGURE_DEPENDS ${RPK_APPLICATION_DIR}/src/*.cpp)
list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/app/Main\\.cpp$")
if(NOT RAYTRACING_ENABLED)
    list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/Raytrace\\.cpp$")
    list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/OptionsGroup(Raytracing|RaytracingMethod|StochasticRelaxationRadiosity|RandomWalkRadiosity|StochasticRaytracing|BidirectionalRaytracing|PhotonMap|RayMatter)\\.cpp$")
endif()

file(GLOB REPLAY_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(rayCorpusReplay ${REPLAY_SRC} ${RPK_APPLICATION_SRC})

target_include_directories(rayCorpusReplay
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${RPK_APPLICATION_DIR}/src
        ${VITRAL_INCLUDE_DIR}
)

set_target_properties(rayCorpusReplay PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(rayCorpusReplay PRIVATE _FILE_OFFSET_BITS=64)

if(RAYTRACING_ENABLED)
    target_compile_definitions(rayCorpusReplay PRIVATE RAYTRACING_ENABLED)
endif()

if(MGF_ENABLED)
    target_compile_definitions(rayCorpusReplay PRIVATE MGF_ENABLED)
endif()

if(TARGET vitral-turboc)
    target_link_libraries(rayCorpusReplay PRIVATE vitral-turboc)
else()
    add_library(vitral-turboc SHARED IMPORTED)
    set_target_properties(vitral-turboc PROPERTIES
        IMPORTED_LOCATION ${VITRAL_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${VITRAL_INCLUDE_DIR}
    )
    target_link_libraries(rayCorpusReplay PRIVATE vitral-turboc)
endif()
