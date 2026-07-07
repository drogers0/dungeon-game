#include "resource_path.h"
#include <filesystem>

std::string resource_path = "assets/";
std::string exe_dir_path;

void initResourcePath(const char* argv0) {
    if (!argv0)
        return;
    namespace fs = std::filesystem;
    fs::path exeDir = fs::weakly_canonical(fs::path(argv0)).parent_path();
    exe_dir_path = exeDir.string();
    if (fs::exists(exeDir / "assets")) {
        // Append the separator after .string() so the trailing slash survives on
        // MSVC std::filesystem too (call sites do `resource_path + "file"`).
        resource_path = (exeDir / "assets").string() + "/";
    }
}
