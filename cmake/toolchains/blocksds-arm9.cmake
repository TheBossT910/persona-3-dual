set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED P3D_BLOCKSDS_ROOT OR P3D_BLOCKSDS_ROOT STREQUAL "")
    if(DEFINED ENV{BLOCKSDS} AND NOT "$ENV{BLOCKSDS}" STREQUAL "")
        set(P3D_BLOCKSDS_ROOT "$ENV{BLOCKSDS}" CACHE PATH "Path to BlocksDS core root")
    elseif(EXISTS "/opt/wonderful/thirdparty/blocksds/core")
        set(P3D_BLOCKSDS_ROOT "/opt/wonderful/thirdparty/blocksds/core" CACHE PATH "Path to BlocksDS core root")
    else()
        set(P3D_BLOCKSDS_ROOT "/opt/blocksds/core" CACHE PATH "Path to BlocksDS core root")
    endif()
endif()

set(_P3D_TOOLCHAIN_HINTS
    /opt/wonderful/toolchain/gcc-arm-none-eabi/bin
    /opt/blocksds/toolchain/gcc-arm-none-eabi/bin
)

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc HINTS ${_P3D_TOOLCHAIN_HINTS} REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++ HINTS ${_P3D_TOOLCHAIN_HINTS} REQUIRED)
find_program(CMAKE_AR arm-none-eabi-ar HINTS ${_P3D_TOOLCHAIN_HINTS} REQUIRED)
find_program(CMAKE_RANLIB arm-none-eabi-ranlib HINTS ${_P3D_TOOLCHAIN_HINTS} REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy HINTS ${_P3D_TOOLCHAIN_HINTS} REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
