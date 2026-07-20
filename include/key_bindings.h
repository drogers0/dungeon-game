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

// Write all 13 bindings to out in "field = KeyName" format (same as controls.cfg).
// Safe to round-trip through loadBindings.
void saveBindings(const KeyBindings& b, std::ostream& out);

// F11/F12 and the current debug bindings cannot be assigned to player actions.
bool isReservedKey(sf::Keyboard::Key key, const KeyBindings& b);

// Return a copy of b with the binding at rowIdx changed to newKey.
// rowIdx mapping: 0=p1_up 1=p1_down 2=p1_left 3=p1_right 4=p1_attack
//                 5=p2_up 6=p2_down 7=p2_left 8=p2_right 9=p2_attack
// If newKey is already bound to another slot, the two slots are swapped.
// If rowIdx already holds newKey, b is returned unchanged.
KeyBindings applyBindingEdit(KeyBindings b, int rowIdx, sf::Keyboard::Key newKey);
