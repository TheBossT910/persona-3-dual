find_program(P3D_MFORMAT_EXECUTABLE mformat)
find_program(P3D_MCOPY_EXECUTABLE mcopy)

if(NOT P3D_MFORMAT_EXECUTABLE OR NOT P3D_MCOPY_EXECUTABLE)
    message(FATAL_ERROR "mtools not found (mformat/mcopy), required for sdcard image packaging. Install mtools or configure with -DP3D_ENABLE_SDCARD=OFF")
endif()

file(GLOB_RECURSE P3D_SDCARD_DATA_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/data/*
)

set(P3D_SDCARD_IMAGE_PATH ${CMAKE_SOURCE_DIR}/sdcard.img)

add_custom_command(
    OUTPUT ${P3D_SDCARD_IMAGE_PATH}
    COMMAND ${CMAKE_COMMAND}
    -DP3D_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DP3D_PYTHON_EXECUTABLE=${Python3_EXECUTABLE}
    -DP3D_MFORMAT_EXECUTABLE=${P3D_MFORMAT_EXECUTABLE}
    -DP3D_MCOPY_EXECUTABLE=${P3D_MCOPY_EXECUTABLE}
    -P ${CMAKE_SOURCE_DIR}/cmake/PackageSdcard.cmake
    DEPENDS
    ${P3D_SDCARD_DATA_FILES}
    ${P3D_ROM_PATH}
    ${CMAKE_SOURCE_DIR}/cmake/PackageSdcard.cmake
    COMMENT "Packaging sdcard.img from ROM and data directory"
    VERBATIM
)

add_custom_target(p3d_sdcard_image DEPENDS ${P3D_SDCARD_IMAGE_PATH})

if(TARGET p3d_assets)
    add_dependencies(p3d_sdcard_image p3d_assets)
endif()
