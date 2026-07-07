#pragma once
#include "NetworkManager.h"
#include <string>
#include <vector>

// Parse a replay file.  Text format: one line per sim step, five 0/1 fields
// (up down left right attack), whitespace-separated.  Everything from '#' to
// end-of-line is ignored (both full-line and inline comments); blank lines are
// skipped.  A malformed line (wrong field count or a value that is not 0 or 1)
// is skipped with a single std::cerr warning; parsing continues.  Never throws.
std::vector<PlayerInput> loadReplay(const std::string& path);

// Map a PlayerInput onto the five P2 bool variables.
// Extracted as a free function so it can be unit-tested without a Game/window.
void applyInputTo(bool& w, bool& a, bool& s, bool& d, bool& space, const PlayerInput& in);

// Map a PlayerInput onto the five P1 variables.
void applyInputToP1(bool& w, bool& a, bool& s, bool& d, bool& attack, const PlayerInput& in);
