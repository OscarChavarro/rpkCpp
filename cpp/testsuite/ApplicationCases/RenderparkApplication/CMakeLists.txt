cmake_minimum_required(VERSION 3.10)
project(RenderparkApplication LANGUAGES C CXX)

option(WITH_RAYCASTING "Enable raycasting modules" ON)
option(WITH_OPENGL "Enable OpenGL modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-ffast-math -O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(VITRAL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../base REALPATH)
get_filename_component(VITRAL_OPENGL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../opengl REALPATH)
set(VITRAL_INCLUDE_DIR "${VITRAL_BASE_DIR}/src/main" CACHE PATH "Include directory for the vitral library")
set(VITRAL_LIBRARY_PATH "${VITRAL_BASE_DIR}/build/libvitral.so" CACHE FILEPATH "Shared library path for vitral when building standalone")
set(VITRAL_OPENGL_INCLUDE_DIR "${VITRAL_OPENGL_BASE_DIR}/src/main" CACHE PATH "Include directory for the vitral-opengl library")
set(VITRAL_OPENGL_LIBRARY_PATH "${VITRAL_OPENGL_BASE_DIR}/build/libvitral-opengl.so" CACHE FILEPATH "Shared library path for vitral-opengl when building standalone")

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

if(TARGET vitral)
    target_link_libraries(rpk PRIVATE vitral)
else()
    add_library(vitral SHARED IMPORTED)
    set_target_properties(vitral PROPERTIES
        IMPORTED_LOCATION ${VITRAL_LIBRARY_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${VITRAL_INCLUDE_DIR}
    )
    target_link_libraries(rpk PRIVATE vitral)
endif()

if(WITH_OPENGL)
    if(TARGET vitral-opengl)
        target_link_libraries(rpk PRIVATE vitral-opengl)
    else()
        add_library(vitral-opengl SHARED IMPORTED)
        set_target_properties(vitral-opengl PROPERTIES
            IMPORTED_LOCATION ${VITRAL_OPENGL_LIBRARY_PATH}
            INTERFACE_INCLUDE_DIRECTORIES "${VITRAL_OPENGL_INCLUDE_DIR};${VITRAL_INCLUDE_DIR}"
        )
        target_link_libraries(rpk PRIVATE vitral-opengl)
    endif()
endif()
