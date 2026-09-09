set(P3D_SOURCE_DIR "${P3D_SOURCE_DIR}" CACHE PATH "Project source directory")

if(NOT P3D_SOURCE_DIR)
    message(FATAL_ERROR "P3D_SOURCE_DIR must be provided")
endif()

file(GLOB_RECURSE P3D_MUSIC_OUTPUTS
    "${P3D_SOURCE_DIR}/data/music/*.pcm"
)
file(GLOB_RECURSE P3D_VIDEO_OUTPUTS
    "${P3D_SOURCE_DIR}/data/video/*.vid"
)
file(GLOB_RECURSE P3D_GRAPHICS_OUTPUTS
    "${P3D_SOURCE_DIR}/data/graphics/*"
)
file(GLOB_RECURSE P3D_ENVIRONMENT_OUTPUTS
    "${P3D_SOURCE_DIR}/data/environments/*"
)
file(GLOB_RECURSE P3D_DATA_MODEL_OUTPUTS
    "${P3D_SOURCE_DIR}/data/models/*"
)
file(GLOB_RECURSE P3D_FONT_OUTPUTS
    "${P3D_SOURCE_DIR}/data/fonts/*"
)
file(GLOB P3D_MAP_OUTPUTS
    "${P3D_SOURCE_DIR}/source/maps/*.hpp"
)
file(GLOB P3D_MODEL_OUTPUTS
    "${P3D_SOURCE_DIR}/source/models/*.hpp"
)
file(GLOB P3D_DIALOGUE_OUTPUTS
    "${P3D_SOURCE_DIR}/source/dialogue/*_dialogue.cpp"
    "${P3D_SOURCE_DIR}/source/dialogue/*_dialogue.h"
    "${P3D_SOURCE_DIR}/source/dialogue/*_dialogue.hpp"
)
file(GLOB P3D_ASSET_STAMPS
    "${P3D_SOURCE_DIR}/out/build/*/p3d_assets_*.stamp"
)

foreach(P3D_OUTPUT_LIST
        P3D_GRAPHICS_OUTPUTS
        P3D_ENVIRONMENT_OUTPUTS
        P3D_DATA_MODEL_OUTPUTS
        P3D_FONT_OUTPUTS)
    list(FILTER ${P3D_OUTPUT_LIST} EXCLUDE REGEX "/\\.gitkeep$")
endforeach()

set(P3D_GENERATED_OUTPUTS
    ${P3D_MUSIC_OUTPUTS}
    ${P3D_VIDEO_OUTPUTS}
    ${P3D_GRAPHICS_OUTPUTS}
    ${P3D_ENVIRONMENT_OUTPUTS}
    ${P3D_DATA_MODEL_OUTPUTS}
    ${P3D_FONT_OUTPUTS}
    ${P3D_MAP_OUTPUTS}
    ${P3D_MODEL_OUTPUTS}
    ${P3D_DIALOGUE_OUTPUTS}
    ${P3D_ASSET_STAMPS}
)

if(P3D_GENERATED_OUTPUTS)
    file(REMOVE ${P3D_GENERATED_OUTPUTS})
endif()

message(STATUS "Generated asset outputs cleaned")
