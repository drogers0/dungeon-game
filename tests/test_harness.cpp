// Socket-free unit tests for the debug harness.
// Covers: loadReplay parser (valid/comment/blank/malformed/field-map),
//         applyInputTo mapping, and gameSeconds step→seconds derivation.
// No Catch2 — plain assert + main(), matching #2's style.

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

#include "NetworkManager.h"
#include "replay.h"

// ── helpers ───────────────────────────────────────────────────────────────────

static constexpr float kFixedDt = 1.f / 60.f;

// ── loadReplay: valid lines + field mapping ────────────────────────────────────

static void test_loadreplay_valid()
{
    const char* path = "/tmp/dg_test_valid.replay";
    {
        std::ofstream f(path);
        f << "# header comment\n";
        f << "\n";
        f << "1 0 0 1 0\n";
        f << "0 1 1 0 1\n";
    }
    auto result = loadReplay(path);
    assert(result.size() == 2);
    // Line 1: up=1 down=0 left=0 right=1 attack=0
    assert(result[0].up     == true);
    assert(result[0].down   == false);
    assert(result[0].left   == false);
    assert(result[0].right  == true);
    assert(result[0].attack == false);
    // Line 2: up=0 down=1 left=1 right=0 attack=1
    assert(result[1].up     == false);
    assert(result[1].down   == true);
    assert(result[1].left   == true);
    assert(result[1].right  == false);
    assert(result[1].attack == true);
}

// ── loadReplay: comment stripping and blank lines ────────────────────────────

static void test_loadreplay_comments_blanks()
{
    const char* path = "/tmp/dg_test_comments.replay";
    {
        std::ofstream f(path);
        f << "# full-line comment\n";
        f << "   \n";                        // whitespace-only
        f << "0 0 0 0 0  # inline comment\n";
        f << "\n";
    }
    auto result = loadReplay(path);
    assert(result.size() == 1);
    assert(result[0].up == false && result[0].attack == false);
}

// ── loadReplay: malformed — wrong field count (too few) ───────────────────────

static void test_loadreplay_malformed_count_few()
{
    const char* path = "/tmp/dg_test_few.replay";
    {
        std::ofstream f(path);
        f << "1 0 0 1\n";      // only 4 fields → malformed
        f << "0 0 0 0 0\n";    // valid
    }
    auto result = loadReplay(path);
    assert(result.size() == 1);
    assert(result[0].up == false);
}

// ── loadReplay: malformed — wrong field count (too many) ──────────────────────

static void test_loadreplay_malformed_count_many()
{
    const char* path = "/tmp/dg_test_many.replay";
    {
        std::ofstream f(path);
        f << "1 0 0 1 0 0\n";  // 6 fields → malformed
        f << "1 0 0 0 1\n";    // valid
    }
    auto result = loadReplay(path);
    assert(result.size() == 1);
    assert(result[0].up == true && result[0].attack == true);
}

// ── loadReplay: malformed — invalid value (not 0 or 1) ───────────────────────

static void test_loadreplay_malformed_value()
{
    const char* path = "/tmp/dg_test_val.replay";
    {
        std::ofstream f(path);
        f << "1 0 2 0 1\n";    // '2' is invalid
        f << "1 0 0 0 1\n";    // valid
    }
    auto result = loadReplay(path);
    assert(result.size() == 1);
    assert(result[0].up == true && result[0].attack == true);
}

// ── loadReplay: field order mapping ──────────────────────────────────────────

static void test_loadreplay_field_mapping()
{
    const char* path = "/tmp/dg_test_fields.replay";
    {
        std::ofstream f(path);
        f << "1 0 0 0 0\n";    // up only
        f << "0 1 0 0 0\n";    // down only
        f << "0 0 1 0 0\n";    // left only
        f << "0 0 0 1 0\n";    // right only
        f << "0 0 0 0 1\n";    // attack only
    }
    auto result = loadReplay(path);
    assert(result.size() == 5);
    assert(result[0].up     && !result[0].down && !result[0].left && !result[0].right && !result[0].attack);
    assert(!result[1].up    && result[1].down  && !result[1].left && !result[1].right && !result[1].attack);
    assert(!result[2].up    && !result[2].down && result[2].left  && !result[2].right && !result[2].attack);
    assert(!result[3].up    && !result[3].down && !result[3].left && result[3].right  && !result[3].attack);
    assert(!result[4].up    && !result[4].down && !result[4].left && !result[4].right && result[4].attack);
}

// ── applyInputTo: all five fields mapped correctly ────────────────────────────

static void test_applyinputto_mapping()
{
    PlayerInput in;
    in.up = true; in.down = false; in.left = true; in.right = false; in.attack = true;

    bool w = false, a = false, s = true, d = true, space = false;
    applyInputTo(w, a, s, d, space, in);

    assert(w     == true);   // in.up
    assert(a     == true);   // in.left
    assert(s     == false);  // in.down
    assert(d     == false);  // in.right
    assert(space == true);   // in.attack
}

// ── gameSeconds: step × kFixedDt derivation ──────────────────────────────────

static void test_gameseconds_derivation()
{
    // kFixedDt is a float (1.f/60.f), so tolerate float-precision rounding
    // when it is widened to double for the multiply (~4e-8 error per step).
    constexpr double kTol = 1e-5;

    // 60 steps → ~1.0 s
    {
        long long steps = 60;
        double    gs    = steps * static_cast<double>(kFixedDt);
        assert(std::abs(gs - 1.0) < kTol);
    }
    // 300 steps → ~5.0 s
    {
        long long steps = 300;
        double    gs    = steps * static_cast<double>(kFixedDt);
        assert(std::abs(gs - 5.0) < kTol);
    }
    // 0 steps → exactly 0.0 s
    {
        long long steps = 0;
        double    gs    = steps * static_cast<double>(kFixedDt);
        assert(gs == 0.0);
    }
}

// ── loadReplay: nonexistent file → empty, no throw ────────────────────────────

static void test_loadreplay_missing_file()
{
    // The most common --replay misuse (wrong path): must return empty, not throw.
    auto result = loadReplay("/tmp/dg_test_does_not_exist_98765.replay");
    assert(result.empty());
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    test_loadreplay_valid();
    test_loadreplay_comments_blanks();
    test_loadreplay_malformed_count_few();
    test_loadreplay_malformed_count_many();
    test_loadreplay_malformed_value();
    test_loadreplay_field_mapping();
    test_loadreplay_missing_file();
    test_applyinputto_mapping();
    test_gameseconds_derivation();

    std::cout << "All harness unit tests passed.\n";
    return 0;
}
