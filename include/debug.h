#pragma once
#include <string>

struct DebugConfig {
    int frames = 0;
    std::string replayPath;
    std::string replayPathP1;
    int screenshotEvery = 0;
    std::string screenshotDir = ".";

    bool active() const {
        return frames > 0 || !replayPath.empty() || !replayPathP1.empty() || screenshotEvery > 0;
    }
};
