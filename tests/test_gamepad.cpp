// Unit tests for axisToInput (pure function, no hardware, no window).
// pollJoystick is not tested here — it calls sf::Joystick and requires hardware.

#include <catch2/catch_test_macros.hpp>

#include "gamepad.h"

TEST_CASE("axisToInput: all zeros, deadzone 25 → all-false", "[gamepad]") {
    PlayerInput in = axisToInput(0.f, 0.f, 0.f, 0.f, 25.f);
    CHECK(!in.up);
    CHECK(!in.down);
    CHECK(!in.left);
    CHECK(!in.right);
    CHECK(!in.attack);
}

TEST_CASE("axisToInput: axis exactly at deadzone → no direction", "[gamepad]") {
    // |x| == deadzone: condition is x > deadzone, so exactly equal → no fire
    PlayerInput in = axisToInput(25.f, 0.f, 0.f, 0.f, 25.f);
    CHECK(!in.right);
    CHECK(!in.left);
}

TEST_CASE("axisToInput: axis just above deadzone → direction fires", "[gamepad]") {
    PlayerInput in = axisToInput(25.01f, 0.f, 0.f, 0.f, 25.f);
    CHECK(in.right);
    CHECK(!in.left);
}

TEST_CASE("axisToInput: left-stick four directions", "[gamepad]") {
    SECTION("right") {
        PlayerInput in = axisToInput(80.f, 0.f, 0.f, 0.f, 25.f);
        CHECK(in.right);
        CHECK(!in.left);
        CHECK(!in.up);
        CHECK(!in.down);
    }
    SECTION("left") {
        PlayerInput in = axisToInput(-80.f, 0.f, 0.f, 0.f, 25.f);
        CHECK(in.left);
        CHECK(!in.right);
        CHECK(!in.up);
        CHECK(!in.down);
    }
    SECTION("down (+y)") {
        PlayerInput in = axisToInput(0.f, 80.f, 0.f, 0.f, 25.f);
        CHECK(in.down);
        CHECK(!in.up);
        CHECK(!in.left);
        CHECK(!in.right);
    }
    SECTION("up (-y)") {
        PlayerInput in = axisToInput(0.f, -80.f, 0.f, 0.f, 25.f);
        CHECK(in.up);
        CHECK(!in.down);
        CHECK(!in.left);
        CHECK(!in.right);
    }
}

TEST_CASE("axisToInput: POV hat four directions", "[gamepad]") {
    SECTION("pov right") {
        PlayerInput in = axisToInput(0.f, 0.f, 100.f, 0.f, 25.f);
        CHECK(in.right);
        CHECK(!in.left);
    }
    SECTION("pov left") {
        PlayerInput in = axisToInput(0.f, 0.f, -100.f, 0.f, 25.f);
        CHECK(in.left);
        CHECK(!in.right);
    }
    SECTION("pov down (+povY)") {
        PlayerInput in = axisToInput(0.f, 0.f, 0.f, 100.f, 25.f);
        CHECK(in.down);
        CHECK(!in.up);
    }
    SECTION("pov up (-povY)") {
        PlayerInput in = axisToInput(0.f, 0.f, 0.f, -100.f, 25.f);
        CHECK(in.up);
        CHECK(!in.down);
    }
}

TEST_CASE("axisToInput: OR-combine same direction (stick + POV both right)", "[gamepad]") {
    PlayerInput in = axisToInput(80.f, 0.f, 100.f, 0.f, 25.f);
    CHECK(in.right);
    CHECK(!in.left);
}

TEST_CASE("axisToInput: OR-combine opposing directions (stick right, pov left)", "[gamepad]") {
    // Both right and left are set; they cancel in update()'s movement code — pin the behavior.
    PlayerInput in = axisToInput(50.f, 0.f, -50.f, 0.f, 25.f);
    CHECK(in.right);
    CHECK(in.left);
}

TEST_CASE("axisToInput: diagonal (right + up)", "[gamepad]") {
    PlayerInput in = axisToInput(80.f, -80.f, 0.f, 0.f, 25.f);
    CHECK(in.right);
    CHECK(in.up);
    CHECK(!in.left);
    CHECK(!in.down);
}

TEST_CASE("axisToInput: extreme values ±100", "[gamepad]") {
    PlayerInput pos = axisToInput(100.f, 100.f, 100.f, 100.f, 25.f);
    CHECK(pos.right);
    CHECK(pos.down);

    PlayerInput neg = axisToInput(-100.f, -100.f, -100.f, -100.f, 25.f);
    CHECK(neg.left);
    CHECK(neg.up);
}

TEST_CASE("axisToInput: zero deadzone, zero value → right=false (> not >=)", "[gamepad]") {
    PlayerInput in = axisToInput(0.f, 0.f, 0.f, 0.f, 0.f);
    CHECK(!in.right);
}

TEST_CASE("axisToInput: zero deadzone, small positive → right=true", "[gamepad]") {
    // Pins > vs >=: with deadzone=0 a tiny positive value must fire.
    PlayerInput in = axisToInput(0.001f, 0.f, 0.f, 0.f, 0.f);
    CHECK(in.right);
}
