#include "replay.h"
#include <fstream>
#include <iostream>
#include <sstream>

void applyInputTo(bool& w, bool& a, bool& s, bool& d, bool& space, const PlayerInput& in)
{
    w     = in.up;
    a     = in.left;
    s     = in.down;
    d     = in.right;
    space = in.attack;
}

std::vector<PlayerInput> loadReplay(const std::string& path)
{
    std::vector<PlayerInput> result;
    std::ifstream            f(path);
    if (!f.is_open()) {
        std::cerr << "replay: cannot open '" << path << "'\n";
        return result;
    }

    std::string line;
    int         lineNum = 0;
    while (std::getline(f, line)) {
        ++lineNum;

        // Strip trailing comment
        auto hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);

        // Skip blank / whitespace-only lines
        bool blank = true;
        for (char c : line)
            if (!std::isspace(static_cast<unsigned char>(c))) {
                blank = false;
                break;
            }
        if (blank)
            continue;

        // Parse exactly 5 fields, each 0 or 1
        std::istringstream ss(line);
        int                vals[5];
        bool               ok = true;
        for (int i = 0; i < 5 && ok; ++i) {
            if (!(ss >> vals[i]))
                ok = false;
            else if (vals[i] != 0 && vals[i] != 1)
                ok = false;
        }
        // Reject lines with extra content
        if (ok) {
            std::string rest;
            if (ss >> rest)
                ok = false;
        }

        if (!ok) {
            std::cerr << "replay: malformed line " << lineNum << " (skipped)\n";
            continue;
        }

        PlayerInput inp;
        inp.up     = (vals[0] != 0);
        inp.down   = (vals[1] != 0);
        inp.left   = (vals[2] != 0);
        inp.right  = (vals[3] != 0);
        inp.attack = (vals[4] != 0);
        result.push_back(inp);
    }
    return result;
}
