if(NOT EXISTS ${P3D_SPECS_FILE})
    message(FATAL_ERROR "Missing ARM9 specs file: ${P3D_SPECS_FILE}. Set BLOCKSDS/P3D_BLOCKSDS_ROOT correctly.")
endif()

set(P3D_MAXMOD_GEN_DIR ${CMAKE_SOURCE_DIR}/build/${P3D_NAME}/maxmod)
set(P3D_MAXMOD_SOUND_BANK ${P3D_MAXMOD_GEN_DIR}/soundbank.bin)
set(P3D_MAXMOD_SOUND_HEADER ${P3D_MAXMOD_GEN_DIR}/soundbank.h)
set(P3D_MAXMOD_SOUND_BIN_HEADER ${P3D_MAXMOD_GEN_DIR}/soundbank_bin.h)
set(P3D_MAXMOD_SOUND_BIN_C ${P3D_MAXMOD_GEN_DIR}/soundbank_bin.c)
set(P3D_MAXMOD_SOUND_OBJ ${P3D_MAXMOD_GEN_DIR}/soundbank.c.o)
set(P3D_ARM7_MAXMOD_ELF ${P3D_BLOCKSDS_ROOT}/sys/arm7/main_core/arm7_maxmod.elf)
set(P3D_ICON_BMP ${CMAKE_SOURCE_DIR}/icon.bmp)
set(P3D_NDS_BANNER "Persona 3 Dual\;Memento Mori.\;Atlus, The P3D Project")

find_program(P3D_MMUTIL_EXECUTABLE mmutil
    HINTS
    ${P3D_BLOCKSDS_ROOT}/tools/mmutil
    /opt/wonderful/thirdparty/blocksds/core/tools/mmutil
    /opt/blocksds/core/tools/mmutil
)

if(NOT P3D_MMUTIL_EXECUTABLE)
    message(FATAL_ERROR "mmutil not found. Set BLOCKSDS/P3D_BLOCKSDS_ROOT correctly.")
endif()

find_program(P3D_BIN2C_EXECUTABLE bin2c
    HINTS
    ${P3D_BLOCKSDS_ROOT}/tools/bin2c
    /opt/wonderful/thirdparty/blocksds/core/tools/bin2c
    /opt/blocksds/core/tools/bin2c
)

if(NOT P3D_BIN2C_EXECUTABLE)
    message(FATAL_ERROR "bin2c not found. Set BLOCKSDS/P3D_BLOCKSDS_ROOT correctly.")
endif()

find_program(P3D_NDSTOOL_EXECUTABLE ndstool
    HINTS
    ${P3D_BLOCKSDS_ROOT}/tools/ndstool
    /opt/wonderful/thirdparty/blocksds/core/tools/ndstool
    /opt/blocksds/core/tools/ndstool
)

if(NOT P3D_NDSTOOL_EXECUTABLE)
    message(FATAL_ERROR "ndstool not found. Set BLOCKSDS/P3D_BLOCKSDS_ROOT correctly.")
endif()

file(GLOB P3D_SFX_WAV_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/assets/sfx/*.wav
)

if(NOT P3D_SFX_WAV_FILES)
    message(FATAL_ERROR "No sound effects found in assets/sfx/*.wav")
endif()

add_custom_command(
    OUTPUT
    ${P3D_MAXMOD_SOUND_HEADER}
    ${P3D_MAXMOD_SOUND_BANK}
    ${P3D_MAXMOD_SOUND_BIN_HEADER}
    ${P3D_MAXMOD_SOUND_BIN_C}
    ${P3D_MAXMOD_SOUND_OBJ}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${P3D_MAXMOD_GEN_DIR}
    COMMAND ${P3D_MMUTIL_EXECUTABLE} ${P3D_SFX_WAV_FILES} -d
    -o${P3D_MAXMOD_SOUND_BANK}
    -h${P3D_MAXMOD_SOUND_HEADER}
    COMMAND ${P3D_BIN2C_EXECUTABLE} ${P3D_MAXMOD_SOUND_BANK} ${P3D_MAXMOD_GEN_DIR}
    COMMAND ${CMAKE_C_COMPILER} -c -o ${P3D_MAXMOD_SOUND_OBJ} ${P3D_MAXMOD_SOUND_BIN_C}
    DEPENDS ${P3D_SFX_WAV_FILES}
    COMMENT "Generating Maxmod soundbank from assets/sfx"
    VERBATIM
)
add_custom_target(p3d_maxmod_soundbank DEPENDS
    ${P3D_MAXMOD_SOUND_HEADER}
    ${P3D_MAXMOD_SOUND_BIN_HEADER}
    ${P3D_MAXMOD_SOUND_OBJ}
)

set(P3D_LINK_EXTRA_OBJECTS ${P3D_MAXMOD_SOUND_OBJ})

if(P3D_GAME_LIBRARY_SOURCES STREQUAL "")
    message(FATAL_ERROR "No game sources found under source/")
endif()

target_include_directories(p3d_game
    PUBLIC
    ${P3D_BLOCKSDS_INCLUDE_DIRS}
    ${P3D_MAXMOD_GEN_DIR}
)

add_dependencies(p3d_game p3d_maxmod_soundbank)

target_compile_definitions(p3d_game PUBLIC __NDS__ __BLOCKSDS__ ARM9)
target_compile_options(p3d_game PUBLIC
    -Wall
    -mthumb
    -mcpu=arm946e-s+nofp
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(p3d_game PUBLIC -O2 -ggdb -Wno-psabi)
else()
    target_compile_options(p3d_game PUBLIC -O2)
endif()

add_executable(p3d_arm9_elf ${P3D_MAIN_CPP} ${P3D_LINK_EXTRA_OBJECTS})
target_link_libraries(p3d_arm9_elf PRIVATE p3d_game)
set_target_properties(p3d_arm9_elf PROPERTIES
    OUTPUT_NAME ${P3D_NAME}
    SUFFIX .elf
    CXX_EXTENSIONS ON
    LINKER_LANGUAGE C
)

target_link_directories(p3d_arm9_elf PRIVATE ${P3D_BLOCKSDS_LIBRARY_DIRS})
target_link_libraries(p3d_arm9_elf PRIVATE
    -Wl,--start-group
    mm9
    nds9
    stdc++
    c
    -Wl,--end-group
)
target_link_options(p3d_arm9_elf PRIVATE
    -mthumb
    -mcpu=arm946e-s+nofp
    -Wl,-Map,${CMAKE_BINARY_DIR}/${P3D_NAME}.map
    -specs=${P3D_SPECS_FILE}
)

if(TARGET p3d_assets)
    add_dependencies(p3d_arm9_elf p3d_assets)
endif()

add_dependencies(p3d_arm9_elf p3d_maxmod_soundbank)

if(NOT EXISTS ${P3D_ARM7_MAXMOD_ELF})
    message(FATAL_ERROR "ARM7 maxmod ELF not found: ${P3D_ARM7_MAXMOD_ELF}")
endif()

if(NOT EXISTS ${P3D_ICON_BMP})
    message(FATAL_ERROR "Game icon not found: ${P3D_ICON_BMP}")
endif()

add_custom_command(
    OUTPUT ${P3D_ROM_PATH}
    COMMAND ${P3D_NDSTOOL_EXECUTABLE}
    -c ${P3D_ROM_PATH}
    -7 ${P3D_ARM7_MAXMOD_ELF}
    -9 $<TARGET_FILE:p3d_arm9_elf>
    -b ${P3D_ICON_BMP} ${P3D_NDS_BANNER}
    DEPENDS p3d_arm9_elf
    COMMENT "Assembling persona-3-dual.nds via ndstool"
    VERBATIM
)
add_custom_target(p3d_nds_rom DEPENDS ${P3D_ROM_PATH})

if(TARGET p3d_sdcard_image)
    add_dependencies(p3d_sdcard_image p3d_nds_rom)
endif()
