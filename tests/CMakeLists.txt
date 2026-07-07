cmake_minimum_required(VERSION 3.16)

# ── Unit tests (socket-free) ──────────────────────────────────────────────────
add_executable(test_netcode test_netcode.cpp)
target_link_libraries(test_netcode PRIVATE dungeon_lib)

# ── Loopback integration tests ────────────────────────────────────────────────
add_executable(test_loopback test_loopback.cpp)
target_link_libraries(test_loopback PRIVATE dungeon_lib)

# ── Warnings (mirror root settings) ──────────────────────────────────────────
foreach(t test_netcode test_loopback)
    if(MSVC)
        target_compile_options(${t} PRIVATE /W4)
    else()
        target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# ── Register with ctest ───────────────────────────────────────────────────────
add_test(NAME netcode_unit     COMMAND test_netcode)
add_test(NAME netcode_loopback COMMAND test_loopback)
