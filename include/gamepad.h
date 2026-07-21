#pragma once
#include "NetworkManager.h" // PlayerInput
#include <SFML/Window.hpp>  // sf::Joystick

inline constexpr float kJoyDeadzone = 25.0f;
inline constexpr unsigned kAttackButton = 0;

// Pure: maps raw joystick axis values to a movement-only PlayerInput.
//
//   x, y       — left-stick axes      (sf::Joystick::X / Y),    range [-100, 100]
//   povX, povY — D-pad POV axes       (sf::Joystick::PovX / Y), same range
//   deadzone   — absolute threshold; |value| <= deadzone → no input
//
// Directions: +x=right, -x=left, +y=down, -y=up (SFML 2 analog convention).
// Left-stick and POV hat are OR-combined per direction.
// NOTE: sf::Joystick::PovY is inverted on Windows XInput (+100=up, -100=down).
// If D-pad up/down are reversed on Windows, invert povY at the call site.
// Attack is NOT part of this function — it is event-driven via JoystickButtonPressed.
inline PlayerInput axisToInput(float x, float y, float povX, float povY, float deadzone) {
    PlayerInput in;
    in.right = (x > deadzone) || (povX > deadzone);
    in.left = (x < -deadzone) || (povX < -deadzone);
    // NOTE: sf::Joystick::PovY is inverted on Windows XInput (+100=up, -100=down).
    // If D-pad up/down are reversed on Windows, pass -povY here.
    in.down = (y > deadzone) || (povY > deadzone);
    in.up = (y < -deadzone) || (povY < -deadzone);
    return in;
}

// Thin sf::Joystick wrapper. Returns movement-only PlayerInput (attack always false).
//   id — joystick index (0 or 1)
// If id is not connected, returns all-false. No prevAttack state needed.
inline PlayerInput pollJoystick(unsigned id, float deadzone = kJoyDeadzone) {
    // LCOV_EXCL_START — sf::Joystick hardware calls (incl. isConnected); not reachable in headless CI
    if (!sf::Joystick::isConnected(id))
        return {};
    float x = sf::Joystick::getAxisPosition(id, sf::Joystick::X);
    float y = sf::Joystick::getAxisPosition(id, sf::Joystick::Y);
    float povX = sf::Joystick::getAxisPosition(id, sf::Joystick::PovX);
    float povY = sf::Joystick::getAxisPosition(id, sf::Joystick::PovY);
    return axisToInput(x, y, povX, povY, deadzone);
    // LCOV_EXCL_STOP
}
