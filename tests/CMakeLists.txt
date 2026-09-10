set(P3D_GTEST_DIR "${CMAKE_SOURCE_DIR}/libs/googletest" CACHE PATH "Path to vendored GoogleTest for this repository")

if(NOT EXISTS "${P3D_GTEST_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Vendored GoogleTest not found at ${P3D_GTEST_DIR}")
endif()

add_subdirectory("${P3D_GTEST_DIR}" "${CMAKE_BINARY_DIR}/_deps/googletest" EXCLUDE_FROM_ALL)

file(GLOB P3D_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)

add_executable(p3d_tests
    ${P3D_TEST_SOURCES}
)

target_include_directories(p3d_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

target_link_libraries(p3d_tests
    PRIVATE
    p3d_game
    $<$<TARGET_EXISTS:GTest::gtest_main>:GTest::gtest_main>
    $<$<TARGET_EXISTS:gtest_main>:gtest_main>
)

include(GoogleTest)
gtest_discover_tests(p3d_tests)
