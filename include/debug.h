#pragma once
#include "ai_difficulty.h"
#include <string>

struct DebugConfig {
    int frames = 0;
    std::string replayPath;
    std::string replayPathP1;
    int screenshotEvery = 0;
    std::string screenshotDir = ".";
    // When >= 0, Game closes + sets quitToMenu after this many sim steps.
    // Also makes active() return true so vsync/F11 are gated off correctly.
    int quitAtStep = -1;

    // AI opponent for P2 (--ai easy|medium|hard).  Does NOT affect active()
    // so --ai without --frames is a silent no-op for the harness bypass check.
    AiDifficulty ai = AiDifficulty::None;

    // Menu screenshot harness: set to a state name to drive showMenu() directly.
    std::string menuState;

    bool menuMode() const { return !menuState.empty(); }

    bool active() const {
        return frames > 0 || !replayPath.empty() || !replayPathP1.empty() || screenshotEvery > 0 ||
               quitAtStep >= 0;
    }
};
