if(NOT DEFINED P3D_SOURCE_DIR)
    message(FATAL_ERROR "P3D_SOURCE_DIR must be provided")
endif()
if(NOT DEFINED P3D_PYTHON_EXECUTABLE)
    message(FATAL_ERROR "P3D_PYTHON_EXECUTABLE must be provided")
endif()
if(NOT DEFINED P3D_MFORMAT_EXECUTABLE)
    message(FATAL_ERROR "P3D_MFORMAT_EXECUTABLE must be provided")
endif()
if(NOT DEFINED P3D_MCOPY_EXECUTABLE)
    message(FATAL_ERROR "P3D_MCOPY_EXECUTABLE must be provided")
endif()

set(P3D_ROM "${P3D_SOURCE_DIR}/persona-3-dual.nds")
set(P3D_DATA "${P3D_SOURCE_DIR}/data")
set(P3D_SDCARD "${P3D_SOURCE_DIR}/sdcard.img")

if(NOT EXISTS "${P3D_ROM}")
    message(FATAL_ERROR "ROM not found: ${P3D_ROM}. Build the ROM first.")
endif()

if(NOT EXISTS "${P3D_DATA}")
    message(FATAL_ERROR "Data directory not found: ${P3D_DATA}")
endif()

execute_process(
    COMMAND "${P3D_PYTHON_EXECUTABLE}" -c "with open('sdcard.img', 'wb') as f: f.truncate(512 * 1024 * 1024 * 4)"
    WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
    RESULT_VARIABLE create_result
)
if(NOT create_result EQUAL 0)
    message(FATAL_ERROR "Failed to allocate sdcard.img")
endif()

execute_process(
    COMMAND "${P3D_MFORMAT_EXECUTABLE}" -i "${P3D_SDCARD}" -v P3D_SD -F ::
    RESULT_VARIABLE format_result
)
if(NOT format_result EQUAL 0)
    message(FATAL_ERROR "Failed to format sdcard.img")
endif()

execute_process(
    COMMAND "${P3D_MCOPY_EXECUTABLE}" -i "${P3D_SDCARD}" "${P3D_ROM}" ::/
    RESULT_VARIABLE copy_rom_result
)
if(NOT copy_rom_result EQUAL 0)
    message(FATAL_ERROR "Failed to copy ROM into sdcard.img")
endif()

execute_process(
    COMMAND "${P3D_MCOPY_EXECUTABLE}" -s -i "${P3D_SDCARD}" "${P3D_DATA}" ::/
    RESULT_VARIABLE copy_data_result
)
if(NOT copy_data_result EQUAL 0)
    message(FATAL_ERROR "Failed to copy data directory into sdcard.img")
endif()

message(STATUS "P3D sdcard image generated at ${P3D_SDCARD}")
