#include "key_bindings.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

// ── Key name table ─────────────────────────────────────────────────────────────
// Bidirectional: keyFromName / nameFromKey both iterate this array.

static const std::pair<const char*, sf::Keyboard::Key> kKeyTable[] = {
    // Letters
    {"A", sf::Keyboard::A},
    {"B", sf::Keyboard::B},
    {"C", sf::Keyboard::C},
    {"D", sf::Keyboard::D},
    {"E", sf::Keyboard::E},
    {"F", sf::Keyboard::F},
    {"G", sf::Keyboard::G},
    {"H", sf::Keyboard::H},
    {"I", sf::Keyboard::I},
    {"J", sf::Keyboard::J},
    {"K", sf::Keyboard::K},
    {"L", sf::Keyboard::L},
    {"M", sf::Keyboard::M},
    {"N", sf::Keyboard::N},
    {"O", sf::Keyboard::O},
    {"P", sf::Keyboard::P},
    {"Q", sf::Keyboard::Q},
    {"R", sf::Keyboard::R},
    {"S", sf::Keyboard::S},
    {"T", sf::Keyboard::T},
    {"U", sf::Keyboard::U},
    {"V", sf::Keyboard::V},
    {"W", sf::Keyboard::W},
    {"X", sf::Keyboard::X},
    {"Y", sf::Keyboard::Y},
    {"Z", sf::Keyboard::Z},
    // Top-row digits
    {"0", sf::Keyboard::Num0},
    {"1", sf::Keyboard::Num1},
    {"2", sf::Keyboard::Num2},
    {"3", sf::Keyboard::Num3},
    {"4", sf::Keyboard::Num4},
    {"5", sf::Keyboard::Num5},
    {"6", sf::Keyboard::Num6},
    {"7", sf::Keyboard::Num7},
    {"8", sf::Keyboard::Num8},
    {"9", sf::Keyboard::Num9},
    // Numpad
    {"Num0", sf::Keyboard::Numpad0},
    {"Num1", sf::Keyboard::Numpad1},
    {"Num2", sf::Keyboard::Numpad2},
    {"Num3", sf::Keyboard::Numpad3},
    {"Num4", sf::Keyboard::Numpad4},
    {"Num5", sf::Keyboard::Numpad5},
    {"Num6", sf::Keyboard::Numpad6},
    {"Num7", sf::Keyboard::Numpad7},
    {"Num8", sf::Keyboard::Numpad8},
    {"Num9", sf::Keyboard::Numpad9},
    // Arrow keys
    {"Up", sf::Keyboard::Up},
    {"Down", sf::Keyboard::Down},
    {"Left", sf::Keyboard::Left},
    {"Right", sf::Keyboard::Right},
    // Special keys
    {"Space", sf::Keyboard::Space},
    {"Enter", sf::Keyboard::Return},
    {"Escape", sf::Keyboard::Escape},
    {"Tab", sf::Keyboard::Tab},
    {"Backspace", sf::Keyboard::BackSpace},
    // Function keys
    {"F1", sf::Keyboard::F1},
    {"F2", sf::Keyboard::F2},
    {"F3", sf::Keyboard::F3},
    {"F4", sf::Keyboard::F4},
    {"F5", sf::Keyboard::F5},
    {"F6", sf::Keyboard::F6},
    {"F7", sf::Keyboard::F7},
    {"F8", sf::Keyboard::F8},
    {"F9", sf::Keyboard::F9},
    {"F10", sf::Keyboard::F10},
    {"F11", sf::Keyboard::F11},
    {"F12", sf::Keyboard::F12},
};

sf::Keyboard::Key keyFromName(const std::string& name) {
    for (const auto& [n, k] : kKeyTable)
        if (name == n)
            return k;
    return sf::Keyboard::Unknown;
}

std::string nameFromKey(sf::Keyboard::Key key) {
    for (const auto& [n, k] : kKeyTable)
        if (k == key)
            return n;
    return "Unknown";
}

// ── Defaults ───────────────────────────────────────────────────────────────────

KeyBindings defaultBindings() {
    KeyBindings b;
    // P1 (rocket) — numpad
    b.p1.up = sf::Keyboard::Numpad8;
    b.p1.down = sf::Keyboard::Numpad5;
    b.p1.left = sf::Keyboard::Numpad4;
    b.p1.right = sf::Keyboard::Numpad6;
    b.p1.attack = sf::Keyboard::Right;
    // P2 (robot) — WASD + Space
    b.p2.up = sf::Keyboard::W;
    b.p2.down = sf::Keyboard::S;
    b.p2.left = sf::Keyboard::A;
    b.p2.right = sf::Keyboard::D;
    b.p2.attack = sf::Keyboard::Space;
    // Debug/speed keys
    b.slowDown = sf::Keyboard::O;
    b.speedUp = sf::Keyboard::P;
    b.skipCooldown = sf::Keyboard::K;
    return b;
}

// ── loadBindings ──────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

KeyBindings loadBindings(std::istream& in) {
    KeyBindings b = defaultBindings();
    std::string line;
    while (std::getline(in, line)) {
        // Strip comment
        auto cpos = line.find('#');
        if (cpos != std::string::npos)
            line = line.substr(0, cpos);
        line = trim(line);
        if (line.empty())
            continue;
        auto eqpos = line.find('=');
        if (eqpos == std::string::npos)
            continue;
        std::string field = trim(line.substr(0, eqpos));
        std::string val = trim(line.substr(eqpos + 1));
        if (val.empty())
            continue;

        sf::Keyboard::Key k = keyFromName(val);
        if (k == sf::Keyboard::Unknown) {
            std::cerr << "[controls.cfg] Unknown key name '" << val << "' for field '" << field
                      << "', keeping default\n";
            continue; // keep default
        }

        if (field == "p1_up")
            b.p1.up = k;
        else if (field == "p1_down")
            b.p1.down = k;
        else if (field == "p1_left")
            b.p1.left = k;
        else if (field == "p1_right")
            b.p1.right = k;
        else if (field == "p1_attack")
            b.p1.attack = k;
        else if (field == "p2_up")
            b.p2.up = k;
        else if (field == "p2_down")
            b.p2.down = k;
        else if (field == "p2_left")
            b.p2.left = k;
        else if (field == "p2_right")
            b.p2.right = k;
        else if (field == "p2_attack")
            b.p2.attack = k;
        else if (field == "slow_down")
            b.slowDown = k;
        else if (field == "speed_up")
            b.speedUp = k;
        else if (field == "skip_cooldown")
            b.skipCooldown = k;
        // Unknown fields silently ignored (forward-compat)
    }
    return b;
}

// ── saveBindings ──────────────────────────────────────────────────────────────

void saveBindings(const KeyBindings& b, std::ostream& out) {
    out << "# Dungeon Game key bindings\n";
    out << "p1_up = " << nameFromKey(b.p1.up) << "\n";
    out << "p1_down = " << nameFromKey(b.p1.down) << "\n";
    out << "p1_left = " << nameFromKey(b.p1.left) << "\n";
    out << "p1_right = " << nameFromKey(b.p1.right) << "\n";
    out << "p1_attack = " << nameFromKey(b.p1.attack) << "\n";
    out << "p2_up = " << nameFromKey(b.p2.up) << "\n";
    out << "p2_down = " << nameFromKey(b.p2.down) << "\n";
    out << "p2_left = " << nameFromKey(b.p2.left) << "\n";
    out << "p2_right = " << nameFromKey(b.p2.right) << "\n";
    out << "p2_attack = " << nameFromKey(b.p2.attack) << "\n";
    out << "slow_down = " << nameFromKey(b.slowDown) << "\n";
    out << "speed_up = " << nameFromKey(b.speedUp) << "\n";
    out << "skip_cooldown = " << nameFromKey(b.skipCooldown) << "\n";
}

// ── applyBindingEdit ──────────────────────────────────────────────────────────

KeyBindings applyBindingEdit(KeyBindings b, int rowIdx, sf::Keyboard::Key newKey) {
    sf::Keyboard::Key* slots[10] = {
        &b.p1.up, &b.p1.down, &b.p1.left, &b.p1.right, &b.p1.attack,
        &b.p2.up, &b.p2.down, &b.p2.left, &b.p2.right, &b.p2.attack,
    };
    if (*slots[rowIdx] == newKey)
        return b;
    // Scan for conflict and swap if found.
    for (int i = 0; i < 10; ++i) {
        if (i != rowIdx && *slots[i] == newKey) {
            *slots[i] = *slots[rowIdx];
            break;
        }
    }
    *slots[rowIdx] = newKey;
    return b;
}
