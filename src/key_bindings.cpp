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
    {"A", sf::Keyboard::Key::A},
    {"B", sf::Keyboard::Key::B},
    {"C", sf::Keyboard::Key::C},
    {"D", sf::Keyboard::Key::D},
    {"E", sf::Keyboard::Key::E},
    {"F", sf::Keyboard::Key::F},
    {"G", sf::Keyboard::Key::G},
    {"H", sf::Keyboard::Key::H},
    {"I", sf::Keyboard::Key::I},
    {"J", sf::Keyboard::Key::J},
    {"K", sf::Keyboard::Key::K},
    {"L", sf::Keyboard::Key::L},
    {"M", sf::Keyboard::Key::M},
    {"N", sf::Keyboard::Key::N},
    {"O", sf::Keyboard::Key::O},
    {"P", sf::Keyboard::Key::P},
    {"Q", sf::Keyboard::Key::Q},
    {"R", sf::Keyboard::Key::R},
    {"S", sf::Keyboard::Key::S},
    {"T", sf::Keyboard::Key::T},
    {"U", sf::Keyboard::Key::U},
    {"V", sf::Keyboard::Key::V},
    {"W", sf::Keyboard::Key::W},
    {"X", sf::Keyboard::Key::X},
    {"Y", sf::Keyboard::Key::Y},
    {"Z", sf::Keyboard::Key::Z},
    // Top-row digits
    {"0", sf::Keyboard::Key::Num0},
    {"1", sf::Keyboard::Key::Num1},
    {"2", sf::Keyboard::Key::Num2},
    {"3", sf::Keyboard::Key::Num3},
    {"4", sf::Keyboard::Key::Num4},
    {"5", sf::Keyboard::Key::Num5},
    {"6", sf::Keyboard::Key::Num6},
    {"7", sf::Keyboard::Key::Num7},
    {"8", sf::Keyboard::Key::Num8},
    {"9", sf::Keyboard::Key::Num9},
    // Numpad
    {"Num0", sf::Keyboard::Key::Numpad0},
    {"Num1", sf::Keyboard::Key::Numpad1},
    {"Num2", sf::Keyboard::Key::Numpad2},
    {"Num3", sf::Keyboard::Key::Numpad3},
    {"Num4", sf::Keyboard::Key::Numpad4},
    {"Num5", sf::Keyboard::Key::Numpad5},
    {"Num6", sf::Keyboard::Key::Numpad6},
    {"Num7", sf::Keyboard::Key::Numpad7},
    {"Num8", sf::Keyboard::Key::Numpad8},
    {"Num9", sf::Keyboard::Key::Numpad9},
    // Arrow keys
    {"Up", sf::Keyboard::Key::Up},
    {"Down", sf::Keyboard::Key::Down},
    {"Left", sf::Keyboard::Key::Left},
    {"Right", sf::Keyboard::Key::Right},
    // Special keys
    {"Space", sf::Keyboard::Key::Space},
    {"Enter", sf::Keyboard::Key::Enter},
    {"Escape", sf::Keyboard::Key::Escape},
    {"Tab", sf::Keyboard::Key::Tab},
    {"Backspace", sf::Keyboard::Key::Backspace},
    // Function keys
    {"F1", sf::Keyboard::Key::F1},
    {"F2", sf::Keyboard::Key::F2},
    {"F3", sf::Keyboard::Key::F3},
    {"F4", sf::Keyboard::Key::F4},
    {"F5", sf::Keyboard::Key::F5},
    {"F6", sf::Keyboard::Key::F6},
    {"F7", sf::Keyboard::Key::F7},
    {"F8", sf::Keyboard::Key::F8},
    {"F9", sf::Keyboard::Key::F9},
    {"F10", sf::Keyboard::Key::F10},
    {"F11", sf::Keyboard::Key::F11},
    {"F12", sf::Keyboard::Key::F12},
};

sf::Keyboard::Key keyFromName(const std::string& name) {
    for (const auto& [n, k] : kKeyTable)
        if (name == n)
            return k;
    return sf::Keyboard::Key::Unknown;
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
    b.p1.up = sf::Keyboard::Key::Numpad8;
    b.p1.down = sf::Keyboard::Key::Numpad5;
    b.p1.left = sf::Keyboard::Key::Numpad4;
    b.p1.right = sf::Keyboard::Key::Numpad6;
    b.p1.attack = sf::Keyboard::Key::Right;
    // P2 (robot) — WASD + Space
    b.p2.up = sf::Keyboard::Key::W;
    b.p2.down = sf::Keyboard::Key::S;
    b.p2.left = sf::Keyboard::Key::A;
    b.p2.right = sf::Keyboard::Key::D;
    b.p2.attack = sf::Keyboard::Key::Space;
    // Debug/speed keys
    b.slowDown = sf::Keyboard::Key::O;
    b.speedUp = sf::Keyboard::Key::P;
    b.skipCooldown = sf::Keyboard::Key::K;
    return b;
}

bool isReservedKey(sf::Keyboard::Key key, const KeyBindings& b) {
    return key == sf::Keyboard::Key::F11 || key == sf::Keyboard::Key::F12 || key == b.slowDown ||
           key == b.speedUp || key == b.skipCooldown;
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
        if (k == sf::Keyboard::Key::Unknown) {
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
