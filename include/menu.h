#pragma once
#include "NetworkManager.h"
#include "ai_difficulty.h"
#include "debug.h"
#include "key_bindings.h"
#include "menu_layout.h"
#include <memory>
#include <string>

struct MenuResult {
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager;
    bool quit = false;
    AiDifficulty ai = AiDifficulty::None;
};

MenuResult showMenu(KeyBindings& bindings, const DebugConfig& cfg = {});

// Map a --menu-state name ("main_menu", "host_waiting", …) to its enum.
// Returns false for an unknown name. Single source of truth for valid state names.
bool parseMenuState(const std::string& name, MenuState& out);
