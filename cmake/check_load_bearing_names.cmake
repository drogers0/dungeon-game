cmake_minimum_required(VERSION 3.16)

# ── Load-bearing identifier guard ─────────────────────────────────────────────
# A few identifiers in this tree are deliberately odd and documented as
# "do not rename" under CLAUDE.md → "Load-bearing quirks". A *partial* rename is
# already a compile error, but a complete, consistent rename would sail through
# every other gate — so check the names are still there. One surviving mention
# per file is enough: anything less than a full rename won't build.
#
# Run: cmake -DSOURCE_DIR=<repo root> -P cmake/check_load_bearing_names.cmake

if(NOT SOURCE_DIR)
    message(FATAL_ERROR "check_load_bearing_names.cmake requires -DSOURCE_DIR=<repo root>")
endif()

# "<path relative to repo root>|<identifier that must still appear in it>"
set(GUARDED_IDENTIFIERS
    "src/Game.cpp|apieceofcrap"   # minutes accumulator; deliberately restored in #30
)

set(failures "")
foreach(entry IN LISTS GUARDED_IDENTIFIERS)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 relpath)
    list(GET parts 1 identifier)

    if(NOT EXISTS "${SOURCE_DIR}/${relpath}")
        list(APPEND failures "${relpath} does not exist (moved or deleted?) — '${identifier}' cannot be verified")
        continue()
    endif()

    file(READ "${SOURCE_DIR}/${relpath}" contents)
    if(NOT contents MATCHES "(^|[^A-Za-z0-9_])${identifier}([^A-Za-z0-9_]|$)")
        list(APPEND failures "${relpath} no longer mentions '${identifier}'")
    endif()
endforeach()

if(failures)
    set(report "")
    foreach(f IN LISTS failures)
        string(APPEND report "\n  - ${f}")
    endforeach()
    message(FATAL_ERROR
        "Load-bearing identifier check failed:${report}\n"
        "These names are intentional. See CLAUDE.md → \"Load-bearing quirks\". If a rename is "
        "genuinely wanted, update GUARDED_IDENTIFIERS in cmake/check_load_bearing_names.cmake "
        "and the CLAUDE.md entry in the same change.")
endif()

message(STATUS "Load-bearing identifiers OK")
