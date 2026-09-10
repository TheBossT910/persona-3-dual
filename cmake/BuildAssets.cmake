if(NOT DEFINED P3D_SOURCE_DIR)
    message(FATAL_ERROR "P3D_SOURCE_DIR must be provided")
endif()

if(NOT DEFINED P3D_PYTHON_EXECUTABLE)
    message(FATAL_ERROR "P3D_PYTHON_EXECUTABLE must be provided")
endif()

if(NOT DEFINED P3D_FFMPEG_EXECUTABLE)
    message(FATAL_ERROR "P3D_FFMPEG_EXECUTABLE must be provided")
endif()

if(NOT DEFINED P3D_GRIT_EXECUTABLE)
    message(FATAL_ERROR "P3D_GRIT_EXECUTABLE must be provided")
endif()

set(ASSETS_DIR "${P3D_SOURCE_DIR}/assets")
set(DATA_DIR "${P3D_SOURCE_DIR}/data")
set(SOURCE_DIR "${P3D_SOURCE_DIR}/source")
set(TOOLS_DIR "${P3D_SOURCE_DIR}/tools")

if(NOT DEFINED P3D_ASSET_GROUP)
    set(P3D_ASSET_GROUP "all")
endif()

string(TOLOWER "${P3D_ASSET_GROUP}" P3D_ASSET_GROUP)

function(p3d_group_enabled group out_var)
    if(P3D_ASSET_GROUP STREQUAL "all" OR P3D_ASSET_GROUP STREQUAL "${group}")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(p3d_run)
    set(options)
    set(oneValueArgs WORKING_DIRECTORY)
    set(multiValueArgs COMMAND)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT ARG_COMMAND)
        message(FATAL_ERROR "p3d_run requires COMMAND")
    endif()

    execute_process(
        COMMAND ${ARG_COMMAND}
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE p3d_result
    )

    if(NOT p3d_result EQUAL 0)
        message(FATAL_ERROR "Command failed with exit code ${p3d_result}: ${ARG_COMMAND}")
    endif()
endfunction()

p3d_group_enabled(dialogue P3D_RUN_DIALOGUE)

if(P3D_RUN_DIALOGUE)
    # Dialogue .dlg -> source/dialogue/*_dialogue.cpp
    file(GLOB DIALOGUE_FILES "${ASSETS_DIR}/dialogue/*.dlg")

    foreach(dlg IN LISTS DIALOGUE_FILES)
        get_filename_component(stem "${dlg}" NAME_WE)
        file(MAKE_DIRECTORY "${SOURCE_DIR}/dialogue")
        p3d_run(
            WORKING_DIRECTORY "${SOURCE_DIR}/dialogue"
            COMMAND "${P3D_PYTHON_EXECUTABLE}" "${TOOLS_DIR}/build_asset.py" "${dlg}" "${stem}"
        )
    endforeach()
endif()

p3d_group_enabled(music P3D_RUN_MUSIC)

if(P3D_RUN_MUSIC)
    # Music .mp3 -> data/music/**/*.pcm
    file(GLOB_RECURSE MP3_FILES "${ASSETS_DIR}/music/*.mp3")

    foreach(mp3 IN LISTS MP3_FILES)
        file(RELATIVE_PATH rel "${ASSETS_DIR}/music" "${mp3}")
        string(REGEX REPLACE "\\.mp3$" ".pcm" rel_pcm "${rel}")
        set(out_pcm "${DATA_DIR}/music/${rel_pcm}")
        get_filename_component(out_dir "${out_pcm}" DIRECTORY)
        file(MAKE_DIRECTORY "${out_dir}")
        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_FFMPEG_EXECUTABLE}" -i "${mp3}" -f s16le -ar 32000 -ac 2 "${out_pcm}" -y -loglevel error
        )
    endforeach()
endif()

p3d_group_enabled(video P3D_RUN_VIDEO)

if(P3D_RUN_VIDEO)
    # Video .mp4 -> data/video/*.vid
    file(GLOB VIDEO_FILES "${ASSETS_DIR}/video/*.mp4")

    foreach(mp4 IN LISTS VIDEO_FILES)
        get_filename_component(stem "${mp4}" NAME_WE)
        file(MAKE_DIRECTORY "${DATA_DIR}/video")
        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_PYTHON_EXECUTABLE}" "${TOOLS_DIR}/build_asset.py" "${mp4}" "${DATA_DIR}/video/${stem}"
        )
    endforeach()
endif()

p3d_group_enabled(environments P3D_RUN_ENVIRONMENTS)

if(P3D_RUN_ENVIRONMENTS)
    # Environments .obj -> data/environments/<name> + sentinel
    file(GLOB ENV_OBJ_FILES "${ASSETS_DIR}/environments/*/*.obj")

    foreach(obj IN LISTS ENV_OBJ_FILES)
        get_filename_component(env_dir "${obj}" DIRECTORY)
        get_filename_component(env_name "${env_dir}" NAME)
        set(out_dir "${DATA_DIR}/environments/${env_name}")
        file(MAKE_DIRECTORY "${out_dir}")
        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_PYTHON_EXECUTABLE}" "${TOOLS_DIR}/build_asset.py" "${obj}" "${out_dir}"
        )
        file(TOUCH "${out_dir}/.sentinel")
    endforeach()
endif()

p3d_group_enabled(models P3D_RUN_MODELS)

if(P3D_RUN_MODELS)
    # Models .json -> source/models/*.hpp + data/models/<name>/<name>.bin
    file(GLOB MODEL_JSON_FILES "${ASSETS_DIR}/models/*/*.json")
    file(GLOB LEGACY_MODEL_HEADERS
        "${SOURCE_DIR}/models/*.h"
        "${SOURCE_DIR}/models/*.hh"
        "${SOURCE_DIR}/models/*.hxx"
    )

    if(LEGACY_MODEL_HEADERS)
        file(REMOVE ${LEGACY_MODEL_HEADERS})
    endif()

    foreach(model_json IN LISTS MODEL_JSON_FILES)
        get_filename_component(model_dir "${model_json}" DIRECTORY)
        get_filename_component(model_name "${model_dir}" NAME)

        set(model_out_dir "${DATA_DIR}/models/${model_name}")
        set(model_bin "${model_out_dir}/${model_name}.bin")
        set(model_h_tmp "${model_out_dir}/${model_name}.hpp")
        set(model_h_dst "${SOURCE_DIR}/models/${model_name}.hpp")

        file(MAKE_DIRECTORY "${model_out_dir}")
        file(MAKE_DIRECTORY "${SOURCE_DIR}/models")

        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_PYTHON_EXECUTABLE}" "${TOOLS_DIR}/build_asset.py" "${model_json}" "${model_bin}"
        )

        if(EXISTS "${model_h_tmp}")
            file(RENAME "${model_h_tmp}" "${model_h_dst}")
        endif()

        file(TOUCH "${model_h_dst}")
    endforeach()
endif()

p3d_group_enabled(maps P3D_RUN_MAPS)

if(P3D_RUN_MAPS)
    file(GLOB LEGACY_MAP_HEADERS
        "${SOURCE_DIR}/maps/*.h"
        "${SOURCE_DIR}/maps/*.hh"
        "${SOURCE_DIR}/maps/*.hxx"
    )

    if(LEGACY_MAP_HEADERS)
        file(REMOVE ${LEGACY_MAP_HEADERS})
    endif()

    file(GLOB JMAP_FILES "${ASSETS_DIR}/maps/*.jmap")

    foreach(jmap IN LISTS JMAP_FILES)
        get_filename_component(stem "${jmap}" NAME_WE)
        set(jmap_out "${SOURCE_DIR}/maps/${stem}.hpp")
        file(MAKE_DIRECTORY "${SOURCE_DIR}/maps")
        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_PYTHON_EXECUTABLE}" "${TOOLS_DIR}/build_asset.py" "${jmap}" "${jmap_out}"
        )
    endforeach()
endif()

p3d_group_enabled(graphics P3D_RUN_GRAPHICS)

if(P3D_RUN_GRAPHICS)
    # UI graphics PNG -> data/graphics/**/<name>/<name>.img.bin/.map.bin/.pal.bin using grit.
    # Nested self-named folder matches every UI view's loadGraphic(path + "name/name") call.
    file(GLOB_RECURSE UI_PNG_FILES "${ASSETS_DIR}/graphics/*.png")

    foreach(png IN LISTS UI_PNG_FILES)
        file(RELATIVE_PATH rel "${ASSETS_DIR}" "${png}")
        get_filename_component(rel_dir "${rel}" DIRECTORY)
        get_filename_component(stem "${png}" NAME_WE)

        set(out_dir "${DATA_DIR}/${rel_dir}/${stem}")
        set(out_base "${out_dir}/${stem}")
        file(MAKE_DIRECTORY "${out_dir}")

        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_GRIT_EXECUTABLE}" "${png}" -ftb -fh! -o "${out_base}"
        )
    endforeach()

    # Environment/model texture paths resolve through IOManager's self-named fallback.
    file(GLOB_RECURSE ENV_MODEL_PNG_FILES
        "${ASSETS_DIR}/environments/*.png"
        "${ASSETS_DIR}/models/*.png"
    )

    foreach(png IN LISTS ENV_MODEL_PNG_FILES)
        file(RELATIVE_PATH rel "${ASSETS_DIR}" "${png}")
        get_filename_component(rel_dir "${rel}" DIRECTORY)
        get_filename_component(stem "${png}" NAME_WE)

        set(out_dir "${DATA_DIR}/${rel_dir}/${stem}")
        set(out_base "${out_dir}/${stem}")
        file(MAKE_DIRECTORY "${out_dir}")

        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_GRIT_EXECUTABLE}" "${png}" -ftb -fh! -o "${out_base}"
        )
    endforeach()
endif()

p3d_group_enabled(fonts P3D_RUN_FONTS)

if(P3D_RUN_FONTS)
    # Font PNG -> data/fonts/**/*.img.bin and .pal.bin using grit with font flags
    file(GLOB_RECURSE FONT_PNG_FILES "${ASSETS_DIR}/fonts/*.png")

    foreach(png IN LISTS FONT_PNG_FILES)
        file(RELATIVE_PATH rel "${ASSETS_DIR}/fonts" "${png}")
        get_filename_component(rel_dir "${rel}" DIRECTORY)
        get_filename_component(stem "${png}" NAME_WE)

        set(out_dir "${DATA_DIR}/fonts/${rel_dir}")
        set(out_base "${out_dir}/${stem}")
        file(MAKE_DIRECTORY "${out_dir}")

        p3d_run(
            WORKING_DIRECTORY "${P3D_SOURCE_DIR}"
            COMMAND "${P3D_GRIT_EXECUTABLE}" "${png}" -ftb -gb -gB8 -fh! -o "${out_base}"
        )
    endforeach()

    # Font metadata copy .fnt -> data/fonts/**/*.fnt
    file(GLOB_RECURSE FONT_FNT_FILES "${ASSETS_DIR}/fonts/*.fnt")

    foreach(fnt IN LISTS FONT_FNT_FILES)
        file(RELATIVE_PATH rel "${ASSETS_DIR}/fonts" "${fnt}")
        set(out_fnt "${DATA_DIR}/fonts/${rel}")
        get_filename_component(out_dir "${out_fnt}" DIRECTORY)
        file(MAKE_DIRECTORY "${out_dir}")
        file(COPY_FILE "${fnt}" "${out_fnt}" ONLY_IF_DIFFERENT)
    endforeach()
endif()

message(STATUS "P3D assets pipeline completed for group: ${P3D_ASSET_GROUP}")
