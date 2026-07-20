#pragma once
#include "NetworkManager.h"
#include "ai_difficulty.h"
#include "debug.h"
#include "menu_layout.h"
#include <memory>

struct MenuResult {
    NetworkMode mode = NetworkMode::LOCAL;
    std::shared_ptr<NetworkManager> netManager;
    bool quit = false;
    AiDifficulty ai = AiDifficulty::None;
};

MenuResult showMenu(const DebugConfig& cfg = {});
