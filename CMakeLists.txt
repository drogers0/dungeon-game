cmake_minimum_required(VERSION 3.16)
project(dungeon_game CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── SFML: fast-path via find_package, fallback to FetchContent ──────────────
find_package(SFML 2.5 COMPONENTS graphics window system audio network QUIET)
if(NOT SFML_FOUND)
    include(FetchContent)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC      OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.2
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SFML)
endif()

# ── dungeon_lib (static): all game logic except main() ──────────────────────
add_library(dungeon_lib STATIC
    src/Game.cpp
    src/NetworkManager.cpp
    src/AnimatedGameObject.cpp
    src/RegularGameObject.cpp
    src/resource_path.cpp
    src/replay.cpp
    src/rng.cpp
    src/key_bindings.cpp
    src/ai.cpp
)

target_include_directories(dungeon_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dungeon_lib PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    sfml-network
)

# ── dungeon_game executable ──────────────────────────────────────────────────
add_executable(dungeon_game src/main.cpp)
target_link_libraries(dungeon_game PRIVATE dungeon_lib)

# Keep console visible on Windows for log output
if(MSVC)
    set_target_properties(dungeon_game PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

# ── Warnings (on our targets only, not SFML) ────────────────────────────────
if(MSVC)
    target_compile_options(dungeon_lib    PRIVATE /W4)
    target_compile_options(dungeon_game   PRIVATE /W4)
else()
    # -Wno-deprecated-declarations: don't let toolchain/SDK deprecations (e.g. newer
    # libc++ deprecating char_traits<unsigned> on bleeding-edge Xcode) fail our -Werror;
    # they're outside our control and vary by compiler version.
    target_compile_options(dungeon_lib    PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-deprecated-declarations)
    target_compile_options(dungeon_game   PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-deprecated-declarations)
endif()

# ── Copy assets next to the binary at build time ────────────────────────────
add_custom_command(TARGET dungeon_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:dungeon_game>/assets
    COMMENT "Copying assets/ to output directory"
)

if(WIN32 AND DEFINED sfml_SOURCE_DIR)
    add_custom_command(TARGET dungeon_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll
            $<TARGET_FILE_DIR:dungeon_game>
        COMMENT "Copying openal32.dll next to dungeon_game.exe")
endif()

# ── Coverage instrumentation (clang only; propagated PUBLIC so test binaries
#    that link dungeon_lib also get the flags) ─────────────────────────────────
option(DUNGEON_COVERAGE "Enable LLVM source-based coverage instrumentation" OFF)
if(DUNGEON_COVERAGE)
    target_compile_options(dungeon_lib PUBLIC -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(dungeon_lib    PUBLIC -fprofile-instr-generate)
endif()

# ── Tests (guarded: only if tests/ subdir exists and option is ON) ───────────
enable_testing()
option(DUNGEON_BUILD_TESTS "Build the dungeon_game test suite" ON)
if(DUNGEON_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
