cmake_minimum_required(VERSION 3.10)
project(RayCorpusReplay LANGUAGES C CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

# Plan step 4, item 4: replays a step 1 ray corpus through the legacy voxel
# grid and VITRAL's spatial indexes over the scene the rpk command line builds.
# It compiles the RenderPark application sources (except its main) with the
# same options, so the scene and the face test are the application's own.

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

get_filename_component(RPK_APPLICATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../RenderparkApplication REALPATH)

file(GLOB RPK_APPLICATION_SRC CONFIGURE_DEPENDS ${RPK_APPLICATION_DIR}/src/*.cpp ${RPK_APPLICATION_DIR}/src/options/*.cpp)
list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/Main\\.cpp$")
if(NOT WITH_RAYCASTING)
    list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/Raytrace\\.cpp$")
    list(FILTER RPK_APPLICATION_SRC EXCLUDE REGEX "/OptionsGroup(Raytracing|RaytracingMethod|StochasticRelaxationRadiosity|RandomWalkRadiosity|StochasticRaytracing|BidirectionalRaytracing|PhotonMap|RayMatter)\\.cpp$")
endif()

file(GLOB REPLAY_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(rayCorpusReplay ${REPLAY_SRC} ${RPK_APPLICATION_SRC})

target_include_directories(rayCorpusReplay
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${RPK_APPLICATION_DIR}/src
)

set_target_properties(rayCorpusReplay PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WITH_RAYCASTING)
    target_compile_definitions(rayCorpusReplay PRIVATE RAYTRACING_ENABLED=true)
endif()

if(WITH_OPENGL)
    target_compile_definitions(rayCorpusReplay PRIVATE OPEN_GL_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(rayCorpusReplay PRIVATE MGF_ENABLED=true)
endif()

find_package(Threads REQUIRED)
target_link_libraries(rayCorpusReplay PRIVATE vitral Threads::Threads)
if(WITH_OPENGL)
    target_link_libraries(rayCorpusReplay PRIVATE vitral-opengl)
endif()
