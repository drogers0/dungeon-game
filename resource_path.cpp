#include "resource_path.h"
#include <filesystem>

std::string resource_path = "elements/";

void initResourcePath(const char* argv0) {
    if (!argv0) return;
    namespace fs = std::filesystem;
    fs::path exeDir = fs::weakly_canonical(fs::path(argv0)).parent_path();
    if (fs::exists(exeDir / "elements")) {
        // Append the separator after .string() so the trailing slash survives on
        // MSVC std::filesystem too (call sites do `resource_path + "file"`).
        resource_path = (exeDir / "elements").string() + "/";
    }
}
