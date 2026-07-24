#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

// Return value from advanceFrameRect: updated rect and updated frame counter.
struct FrameResult {
    sf::IntRect rect;
    int curr;
};

// Compute the next sprite-sheet texture rect and frame counter.
// Reproduces AnimatedGameObject::update()'s exact stepping — including the
// integer ceil(curr/nx) quirk (int division before ceil) and the curr==howmany
// reset — so both can be pinned by unit tests without a window.
//
// Parameters match AnimatedGameObject member names:
//   rect    — current texture rect
//   curr    — current frame index (1-based)
//   nx/ny   — columns / rows in the sheet
//   howmany — total frame count
//   xsize/ysize — full texture dimensions in pixels
inline FrameResult advanceFrameRect(sf::IntRect rect, int curr, int nx, int ny, int howmany,
                                    double xsize, double ysize) {
    if (curr < howmany) {
        int check = curr % nx;
        if (check == 0) {
            rect = sf::IntRect(
                {0, rect.position.y + rect.size.y},
                {(int)std::floor(xsize / (double)nx), (int)std::floor(ysize / (double)ny)});
        } else {
            int h = (int)std::floor(ysize / (double)ny);
            rect = sf::IntRect({rect.position.x + rect.size.x, ((int)std::ceil(curr / nx)) * h},
                               {rect.size.x, (int)(ysize / ny)});
        }
        curr++;
    } else {
        rect = sf::IntRect(
            {0, 0}, {(int)std::floor(xsize / (double)nx), (int)std::floor(ysize / (double)ny)});
        curr = 1;
    }
    return {rect, curr};
}
