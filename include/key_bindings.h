#pragma once
#include <SFML/Window.hpp>
#include <iosfwd>
#include <string>

// Per-player movement and attack bindings.
struct PlayerBindings {
    sf::Keyboard::Key up;
    sf::Keyboard::Key down;
    sf::Keyboard::Key left;
    sf::Keyboard::Key right;
    sf::Keyboard::Key attack;
};

// Full set of key bindings for both players plus debug/speed keys.
struct KeyBindings {
    PlayerBindings p1;
    PlayerBindings p2;
    sf::Keyboard::Key slowDown;
    sf::Keyboard::Key speedUp;
    sf::Keyboard::Key skipCooldown;
};

// Returns the compile-time defaults (original hard-coded keys).
KeyBindings defaultBindings();

// Parse a "key = value" config stream, returning a KeyBindings populated from
// recognised entries.  Unknown SFML key names produce a std::cerr warning and
// keep the existing binding's default.  Unrecognised field names are silently
// ignored.  An absent/empty stream returns defaults untouched.
KeyBindings loadBindings(std::istream& in);

// Convert between SFML key enum and the string names used in controls.cfg.
// keyFromName returns sf::Keyboard::Unknown for unrecognised names.
// nameFromKey returns "Unknown" for keys not in the table.
sf::Keyboard::Key keyFromName(const std::string& name);
std::string nameFromKey(sf::Keyboard::Key key);
