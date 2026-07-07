#pragma once
#include <string>

struct DebugConfig {
    int         frames          = 0;
    std::string replayPath;
    int         screenshotEvery = 0;
    std::string screenshotDir   = ".";

    bool active() const { return frames > 0 || !replayPath.empty() || screenshotEvery > 0; }
};
