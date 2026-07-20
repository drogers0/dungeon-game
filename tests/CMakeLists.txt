cmake_minimum_required(VERSION 3.16)

# ── Catch2 via FetchContent ────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# ── Unit tests (no display required) ─────────────────────────────────────────
add_executable(dungeon_tests
    test_netcode.cpp
    test_harness.cpp
    test_loopback.cpp
    test_unit.cpp
    test_ai.cpp
)
target_link_libraries(dungeon_tests PRIVATE dungeon_lib Catch2::Catch2WithMain)

# ── Integration tests (constructs Game; needs display + assets) ───────────────
add_executable(dungeon_integration_tests
    test_integration.cpp
)
target_link_libraries(dungeon_integration_tests PRIVATE dungeon_lib Catch2::Catch2WithMain)
# Expose tests/data/ path so integration tests can locate replay files portably
target_compile_definitions(dungeon_integration_tests PRIVATE
    TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
# Ensure dungeon_game (and thus assets/ copy) is built before integration tests
add_dependencies(dungeon_integration_tests dungeon_game)

# Static sfml-audio still imports openal32.dll at runtime; copy it next to each
# test binary (they live in their own dir under a multi-config generator) so a
# developer can run either exe / ctest directly without a DLL-load failure.
if(WIN32 AND DEFINED sfml_SOURCE_DIR)
    foreach(t dungeon_tests dungeon_integration_tests)
        add_custom_command(TARGET ${t} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll
                $<TARGET_FILE_DIR:${t}>
            COMMENT "Copying openal32.dll next to ${t}")
    endforeach()
endif()

# ── Warnings (mirror root settings) ──────────────────────────────────────────
foreach(t dungeon_tests dungeon_integration_tests)
    if(MSVC)
        target_compile_options(${t} PRIVATE /W4)
    else()
        target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# ── Register with ctest ───────────────────────────────────────────────────────
catch_discover_tests(dungeon_tests PROPERTIES LABELS "unit")
catch_discover_tests(dungeon_integration_tests
    PROPERTIES LABELS "integration"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dungeon_game>
)

# ── Subprocess smoke test (covers main.cpp CLI) ───────────────────────────────
add_test(NAME smoke
    COMMAND $<TARGET_FILE:dungeon_game>
        --frames 60
        --replay ${CMAKE_CURRENT_SOURCE_DIR}/data/demo.replay
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dungeon_game>
)
set_tests_properties(smoke PROPERTIES LABELS "integration")

# ── Menu harness CLI smoke tests ──────────────────────────────────────────────
add_test(NAME menu_state_cli
    COMMAND $<TARGET_FILE:dungeon_game>
        --menu-state main_menu
        --frames 3
        --screenshot-dir $<TARGET_FILE_DIR:dungeon_game>/menu_smoke
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dungeon_game>
)
set_tests_properties(menu_state_cli PROPERTIES LABELS "integration")

add_test(NAME menu_state_invalid
    COMMAND $<TARGET_FILE:dungeon_game> --menu-state bogus
)
set_tests_properties(menu_state_invalid PROPERTIES WILL_FAIL TRUE LABELS "unit")
