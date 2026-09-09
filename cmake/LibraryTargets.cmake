add_library(p3d_game STATIC ${P3D_GAME_LIBRARY_SOURCES})

if(P3D_BUILD_NDS)
    set(P3D_GENERATED_GAME_SOURCES
        ${P3D_GENERATED_DIALOGUE_SOURCES}
        ${CMAKE_SOURCE_DIR}/source/data/environmentDb.cpp
    )

    if(P3D_GENERATED_GAME_SOURCES)
        list(REMOVE_DUPLICATES P3D_GENERATED_GAME_SOURCES)

        foreach(generated_source IN LISTS P3D_GENERATED_GAME_SOURCES)
            if(generated_source)
                set_source_files_properties(${generated_source} PROPERTIES GENERATED TRUE)
                target_sources(p3d_game PRIVATE ${generated_source})
            endif()
        endforeach()
    endif()
endif()

set_target_properties(p3d_game PROPERTIES CXX_EXTENSIONS ON)
target_compile_features(p3d_game PUBLIC cxx_std_17)

if(TARGET aegis_engine)
    target_link_libraries(p3d_game PUBLIC aegis_engine)
endif()

target_sources(p3d_game
    PUBLIC
    FILE_SET p3d_game_headers TYPE HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/source
    FILES ${P3D_GAME_HEADERS}
)

target_include_directories(p3d_game
    PUBLIC
    ${P3D_ENGINE_INCLUDE_DIRS}
)
