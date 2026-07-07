#pragma once
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

    bool active() const {
        return frames > 0 || !replayPath.empty() || !replayPathP1.empty() || screenshotEvery > 0 ||
               quitAtStep >= 0;
    }
};
