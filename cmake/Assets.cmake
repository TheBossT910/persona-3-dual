find_program(P3D_FFMPEG_EXECUTABLE ffmpeg)

if(NOT P3D_FFMPEG_EXECUTABLE)
    message(FATAL_ERROR "ffmpeg not found, required for media conversion. Install ffmpeg or configure with -DP3D_ENABLE_ASSETS=OFF")
endif()

if(NOT P3D_BLOCKSDS_ROOT)
    set(P3D_BLOCKSDS_ROOT "/opt/blocksds/core")
endif()

find_program(P3D_GRIT_EXECUTABLE grit
    HINTS
    ${P3D_BLOCKSDS_ROOT}/tools/grit
    /opt/blocksds/core/tools/grit
    /opt/wonderful/thirdparty/blocksds/core/tools/grit
)

if(NOT P3D_GRIT_EXECUTABLE)
    message(FATAL_ERROR "grit not found, required for graphics conversion. Set BLOCKSDS/P3D_BLOCKSDS_ROOT or configure with -DP3D_ENABLE_ASSETS=OFF")
endif()

file(GLOB_RECURSE P3D_ASSET_TOOL_INPUTS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/tools/converters/*
)

function(p3d_add_asset_group group_name)
    set(options)
    set(oneValueArgs COMMENT)
    set(multiValueArgs PATTERNS)
    cmake_parse_arguments(P3D_GROUP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT P3D_GROUP_PATTERNS)
        message(FATAL_ERROR "p3d_add_asset_group(${group_name}) requires PATTERNS")
    endif()

    file(GLOB_RECURSE P3D_GROUP_INPUTS CONFIGURE_DEPENDS ${P3D_GROUP_PATTERNS})
    set(P3D_GROUP_STAMP ${CMAKE_BINARY_DIR}/p3d_assets_${group_name}.stamp)
    set(P3D_GROUP_TARGET p3d_assets_${group_name})

    add_custom_command(
        OUTPUT ${P3D_GROUP_STAMP}
        COMMAND ${CMAKE_COMMAND}
        -DP3D_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DP3D_PYTHON_EXECUTABLE=${Python3_EXECUTABLE}
        -DP3D_FFMPEG_EXECUTABLE=${P3D_FFMPEG_EXECUTABLE}
        -DP3D_GRIT_EXECUTABLE=${P3D_GRIT_EXECUTABLE}
        -DP3D_ASSET_GROUP=${group_name}
        -P ${CMAKE_SOURCE_DIR}/cmake/BuildAssets.cmake
        COMMAND ${CMAKE_COMMAND} -E touch ${P3D_GROUP_STAMP}
        DEPENDS
        ${P3D_GROUP_INPUTS}
        ${P3D_ASSET_TOOL_INPUTS}
        ${CMAKE_SOURCE_DIR}/tools/build_asset.py
        ${CMAKE_SOURCE_DIR}/cmake/BuildAssets.cmake
        COMMENT "${P3D_GROUP_COMMENT}"
        VERBATIM
    )

    add_custom_target(${P3D_GROUP_TARGET} DEPENDS ${P3D_GROUP_STAMP})
    set_property(GLOBAL APPEND PROPERTY P3D_ASSET_GROUP_TARGETS ${P3D_GROUP_TARGET})
endfunction()

p3d_add_asset_group(dialogue
    COMMENT "Generating dialogue assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/dialogue/*.dlg
    ${CMAKE_SOURCE_DIR}/assets/dialogue/*.build.json
)
p3d_add_asset_group(music
    COMMENT "Generating music assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/music/*.mp3
)
p3d_add_asset_group(video
    COMMENT "Generating video assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/video/*.mp4
    ${CMAKE_SOURCE_DIR}/assets/video/*.build.json
)
p3d_add_asset_group(environments
    COMMENT "Generating environment assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.obj
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.png
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.mtl
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.build.json
    ${CMAKE_SOURCE_DIR}/assets/environments/*.build.json
)
p3d_add_asset_group(models
    COMMENT "Generating model assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/models/*/*.json
    ${CMAKE_SOURCE_DIR}/assets/models/*/*.png
    ${CMAKE_SOURCE_DIR}/assets/models/*/*.build.json
    ${CMAKE_SOURCE_DIR}/assets/models/*.build.json
)
p3d_add_asset_group(maps
    COMMENT "Generating map assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/maps/*.jmap
)
p3d_add_asset_group(graphics
    COMMENT "Generating graphics assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/graphics/*.png
    ${CMAKE_SOURCE_DIR}/assets/graphics/*.grit
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.png
    ${CMAKE_SOURCE_DIR}/assets/environments/*/*.grit
    ${CMAKE_SOURCE_DIR}/assets/models/*/*.png
    ${CMAKE_SOURCE_DIR}/assets/models/*/*.grit
)
p3d_add_asset_group(fonts
    COMMENT "Generating font assets"
    PATTERNS
    ${CMAKE_SOURCE_DIR}/assets/fonts/*.png
    ${CMAKE_SOURCE_DIR}/assets/fonts/*.fnt
    ${CMAKE_SOURCE_DIR}/assets/fonts/*.grit
)

add_dependencies(p3d_assets_graphics p3d_assets_environments p3d_assets_models)

add_custom_target(p3d_environment_db DEPENDS p3d_assets_environments)

add_custom_target(p3d_assets)
add_custom_target(p3d_clean_assets
    COMMAND ${CMAKE_COMMAND}
    -DP3D_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/CleanAssets.cmake
    COMMENT "Cleaning generated asset outputs"
    VERBATIM
)
get_property(P3D_ASSET_GROUP_TARGETS GLOBAL PROPERTY P3D_ASSET_GROUP_TARGETS)

if(P3D_ASSET_GROUP_TARGETS)
    add_dependencies(p3d_assets ${P3D_ASSET_GROUP_TARGETS})
endif()

add_dependencies(p3d_assets p3d_environment_db)

if(TARGET p3d_game)
    add_dependencies(p3d_game p3d_assets)
endif()
