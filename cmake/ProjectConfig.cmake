set(P3D_NAME "persona-3-dual")
set(P3D_ROM_PATH "${CMAKE_SOURCE_DIR}/${P3D_NAME}.nds")

if(NOT P3D_BLOCKSDS_ROOT)
    set(P3D_BLOCKSDS_ROOT "/opt/blocksds/core")
endif()

set(P3D_SPECS_FILE "${P3D_BLOCKSDS_ROOT}/sys/crts/ds_arm9.specs")

set(P3D_ENGINE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/source
)

set(P3D_BLOCKSDS_INCLUDE_DIRS
    ${P3D_BLOCKSDS_ROOT}/libs/maxmod/include
    ${P3D_BLOCKSDS_ROOT}/libs/libnds/include
)

set(P3D_BLOCKSDS_LIBRARY_DIRS
    ${P3D_BLOCKSDS_ROOT}/libs/maxmod/lib
    ${P3D_BLOCKSDS_ROOT}/libs/libnds/lib
)

set(P3D_PROJECT_VENV_PYTHON "$ENV{HOME}/.venv/bin/python3")

if(EXISTS "${P3D_PROJECT_VENV_PYTHON}")
    set(Python3_EXECUTABLE "${P3D_PROJECT_VENV_PYTHON}" CACHE FILEPATH "Python interpreter for project tools" FORCE)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Source layout rules: generated asset outputs live in source/maps, source/models,
# and source/dialogue and must not be included in the normal source glob.
file(GLOB_RECURSE P3D_GAME_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/source/*.h
    ${CMAKE_SOURCE_DIR}/source/*.hpp
)
file(GLOB_RECURSE P3D_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/source/*.cpp)

file(GLOB P3D_DIALOGUE_INPUTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/dialogue/*.dlg)
set(P3D_GENERATED_DIALOGUE_SOURCES "")

foreach(dlg IN LISTS P3D_DIALOGUE_INPUTS)
    get_filename_component(dialogue_stem "${dlg}" NAME_WE)
    list(APPEND P3D_GENERATED_DIALOGUE_SOURCES "${CMAKE_SOURCE_DIR}/source/dialogue/${dialogue_stem}_dialogue.cpp")
endforeach()

list(FILTER P3D_GAME_HEADERS EXCLUDE REGEX "/source/(maps|models|dialogue)/")
list(FILTER P3D_GAME_SOURCES EXCLUDE REGEX "/source/(maps|models|dialogue)/")

set(P3D_MAIN_CPP ${CMAKE_SOURCE_DIR}/source/main.cpp)

if(P3D_BUILD_NDS)
    set(P3D_GAME_LIBRARY_SOURCES ${P3D_GAME_SOURCES})
    list(REMOVE_ITEM P3D_GAME_LIBRARY_SOURCES ${P3D_MAIN_CPP})
else()
    set(P3D_GAME_LIBRARY_SOURCES ${CMAKE_SOURCE_DIR}/source/battleActions/skills/BattleCalcsCoreTestExample.cpp)
endif()

if(P3D_GAME_LIBRARY_SOURCES STREQUAL "")
    message(FATAL_ERROR "No game sources found for p3d_game")
endif()

if(P3D_USE_BUNDLED_AEGIS_ENGINE)
    if(P3D_BUILD_AEGIS_ENGINE_TESTS)
        set(AEGIS_ENGINE_BUILD_TESTS ON CACHE BOOL "Build Aegis Engine tests" FORCE)
    else()
        set(AEGIS_ENGINE_BUILD_TESTS OFF CACHE BOOL "Build Aegis Engine tests" FORCE)
    endif()

    add_subdirectory(${CMAKE_SOURCE_DIR}/libs/aegis_engine EXCLUDE_FROM_ALL)
endif()
