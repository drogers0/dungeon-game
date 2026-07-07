#pragma once
#include <string>

extern std::string resource_path;
// Directory containing the executable (set by initResourcePath).
// Use this to locate files next to the binary (e.g. controls.cfg).
extern std::string exe_dir_path;
void initResourcePath(const char* argv0);
