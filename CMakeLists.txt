cmake_minimum_required(VERSION 3.23)

project(persona_3_dual LANGUAGES C CXX)

option(P3D_BUILD_TESTS "Build host-side unit tests" ON)
option(P3D_BUILD_NDS "Build Nintendo DS ROM target (WIP)" OFF)
option(P3D_ENABLE_ASSETS "Enable CMake-driven asset generation target" ON)
option(P3D_ENABLE_SDCARD "Enable CMake-driven sdcard packaging target" ON)
option(P3D_USE_BUNDLED_AEGIS_ENGINE "Use bundled libs/aegis_engine CMake target" ON)
option(P3D_BUILD_AEGIS_ENGINE_TESTS "Build bundled Aegis Engine tests from root project" OFF)

set(P3D_BLOCKSDS_ROOT "$ENV{BLOCKSDS}" CACHE PATH "Path to BlocksDS core root")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/ProjectConfig.cmake)
include(cmake/LibraryTargets.cmake)

if(P3D_ENABLE_ASSETS)
    include(cmake/Assets.cmake)
endif()

if(P3D_ENABLE_SDCARD)
    include(cmake/Packaging.cmake)
endif()

if(P3D_BUILD_TESTS OR P3D_BUILD_AEGIS_ENGINE_TESTS)
    enable_testing()
endif()

if(P3D_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(P3D_BUILD_NDS)
    include(cmake/NDS.cmake)
endif()
