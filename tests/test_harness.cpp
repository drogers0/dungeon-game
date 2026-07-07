// Unit tests for the debug harness.
// Covers: loadReplay parser, applyInputTo mapping, gameSeconds derivation.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "NetworkManager.h"
#include "replay.h"

static constexpr float kFixedDt = 1.f / 60.f;

// ── loadReplay: valid lines + field mapping ────────────────────────────────────

TEST_CASE("loadReplay parses valid lines", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_valid.replay").string();
    {
        std::ofstream f(path);
        f << "# header comment\n";
        f << "\n";
        f << "1 0 0 1 0\n";
        f << "0 1 1 0 1\n";
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 2);
    // Line 1: up=1 down=0 left=0 right=1 attack=0
    REQUIRE(result[0].up == true);
    REQUIRE(result[0].down == false);
    REQUIRE(result[0].left == false);
    REQUIRE(result[0].right == true);
    REQUIRE(result[0].attack == false);
    // Line 2: up=0 down=1 left=1 right=0 attack=1
    REQUIRE(result[1].up == false);
    REQUIRE(result[1].down == true);
    REQUIRE(result[1].left == true);
    REQUIRE(result[1].right == false);
    REQUIRE(result[1].attack == true);
}

TEST_CASE("loadReplay strips comments and blank lines", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_comments.replay").string();
    {
        std::ofstream f(path);
        f << "# full-line comment\n";
        f << "   \n";
        f << "0 0 0 0 0  # inline comment\n";
        f << "\n";
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].up == false);
    REQUIRE(result[0].attack == false);
}

TEST_CASE("loadReplay skips malformed line with too few fields", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_few.replay").string();
    {
        std::ofstream f(path);
        f << "1 0 0 1\n";   // only 4 fields
        f << "0 0 0 0 0\n"; // valid
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].up == false);
}

TEST_CASE("loadReplay skips malformed line with too many fields", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_many.replay").string();
    {
        std::ofstream f(path);
        f << "1 0 0 1 0 0\n"; // 6 fields
        f << "1 0 0 0 1\n";   // valid
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].up == true);
    REQUIRE(result[0].attack == true);
}

TEST_CASE("loadReplay skips malformed line with invalid value", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_val.replay").string();
    {
        std::ofstream f(path);
        f << "1 0 2 0 1\n"; // '2' is invalid
        f << "1 0 0 0 1\n"; // valid
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].up == true);
    REQUIRE(result[0].attack == true);
}

TEST_CASE("loadReplay field order: each field in isolation", "[harness][unit]") {
    auto path = (std::filesystem::temp_directory_path() / "dg_test_fields.replay").string();
    {
        std::ofstream f(path);
        f << "1 0 0 0 0\n"; // up only
        f << "0 1 0 0 0\n"; // down only
        f << "0 0 1 0 0\n"; // left only
        f << "0 0 0 1 0\n"; // right only
        f << "0 0 0 0 1\n"; // attack only
    }
    auto result = loadReplay(path.c_str());
    REQUIRE(result.size() == 5);
    REQUIRE(result[0].up);
    REQUIRE(!result[0].down);
    REQUIRE(!result[0].left);
    REQUIRE(!result[0].right);
    REQUIRE(!result[0].attack);
    REQUIRE(!result[1].up);
    REQUIRE(result[1].down);
    REQUIRE(!result[1].left);
    REQUIRE(!result[1].right);
    REQUIRE(!result[1].attack);
    REQUIRE(!result[2].up);
    REQUIRE(!result[2].down);
    REQUIRE(result[2].left);
    REQUIRE(!result[2].right);
    REQUIRE(!result[2].attack);
    REQUIRE(!result[3].up);
    REQUIRE(!result[3].down);
    REQUIRE(!result[3].left);
    REQUIRE(result[3].right);
    REQUIRE(!result[3].attack);
    REQUIRE(!result[4].up);
    REQUIRE(!result[4].down);
    REQUIRE(!result[4].left);
    REQUIRE(!result[4].right);
    REQUIRE(result[4].attack);
}

TEST_CASE("loadReplay nonexistent file returns empty without throw", "[harness][unit]") {
    auto path =
        (std::filesystem::temp_directory_path() / "dg_test_does_not_exist_98765.replay").string();
    auto result = loadReplay(path.c_str());
    REQUIRE(result.empty());
}

// ── applyInputTo: all five fields ─────────────────────────────────────────────

TEST_CASE("applyInputTo maps all five fields correctly", "[harness][unit]") {
    PlayerInput in;
    in.up = true;
    in.down = false;
    in.left = true;
    in.right = false;
    in.attack = true;

    bool w = false, a = false, s = true, d = true, space = false;
    applyInputTo(w, a, s, d, space, in);

    REQUIRE(w == true);     // in.up
    REQUIRE(a == true);     // in.left
    REQUIRE(s == false);    // in.down
    REQUIRE(d == false);    // in.right
    REQUIRE(space == true); // in.attack
}

// ── gameSeconds derivation ────────────────────────────────────────────────────

TEST_CASE("gameSeconds: step * kFixedDt", "[harness][unit]") {
    constexpr double kTol = 1e-5;
    REQUIRE(std::abs(60LL * static_cast<double>(kFixedDt) - 1.0) < kTol);
    REQUIRE(std::abs(300LL * static_cast<double>(kFixedDt) - 5.0) < kTol);
    REQUIRE(0LL * static_cast<double>(kFixedDt) == 0.0);
}
