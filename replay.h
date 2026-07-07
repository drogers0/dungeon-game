#pragma once
#include <string>
#include <vector>
#include "NetworkManager.h"

// Parse a replay file.  Text format: one line per sim step, five 0/1 fields
// (up down left right attack), whitespace-separated.  Blank lines and lines
// beginning with '#' (after optional leading whitespace) are ignored.  A
// malformed line (wrong field count or a value that is not 0 or 1) is skipped
// with a single std::cerr warning; parsing continues.  Never throws.
std::vector<PlayerInput> loadReplay(const std::string& path);

// Map a PlayerInput onto the five P2 bool variables.
// Extracted as a free function so it can be unit-tested without a Game/window.
void applyInputTo(bool& w, bool& a, bool& s, bool& d, bool& space, const PlayerInput& in);
