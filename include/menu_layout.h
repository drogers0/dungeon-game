#pragma once
#include "game_canvas.h"
#include "key_bindings.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Logical canvas for menus / endscreen.
inline constexpr float kMenuW = 1024.f;
inline constexpr float kMenuH = 576.f;

// Standard button geometry.
inline constexpr float kBtnW = 280.f;
inline constexpr float kBtnH = 60.f;
inline constexpr float kBtnGap = 18.f;

// Settings-screen row geometry.
inline constexpr float kSettingsRowW = 380.f;
inline constexpr float kSettingsRowH = 38.f;
inline constexpr float kSettingsRowGap = 8.f;

enum class MenuState {
    MAIN_MENU,
    HOST_WAITING,
    JOIN_INPUT,
    AI_DIFFICULTY,
    READY_TO_START,
    SETTINGS
};

// Returns the Y-center of each button in a vertical stack centered within canvasH.
// Returns empty vector for count <= 0.
inline std::vector<float> stackedButtonCenters(float canvasH, float btnH, float gap, int count) {
    if (count <= 0)
        return {};
    float totalH = count * btnH + (count - 1) * gap;
    float startY = (canvasH - totalH) / 2.f + btnH / 2.f;
    std::vector<float> ys;
    ys.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        ys.push_back(startY + static_cast<float>(i) * (btnH + gap));
    return ys;
}

struct ButtonSpec {
    sf::FloatRect rect;
    std::string label;
};

// Returns ButtonSpec for every clickable button in the given state.
// Rects are in kMenuW x kMenuH logical space.
// MAIN_MENU:       6 stacked (1 PLAYER, 2 PLAYER, HOST, JOIN, CONTROLS, QUIT)
// AI_DIFFICULTY:   3 stacked (EASY, MEDIUM, HARD) + Back at top-left
// HOST_WAITING:    Back only
// JOIN_INPUT:      Back only
// READY_TO_START:  START only (centered, no Back)
// SETTINGS:        (empty — layout driven by settingsButtonSpecs)
inline std::vector<ButtonSpec> menuButtonRects(MenuState state) {
    const sf::FloatRect backRect{{20.f, 20.f}, {120.f, 38.f}};

    if (state == MenuState::SETTINGS)
        return {};

    if (state == MenuState::MAIN_MENU) {
        auto ys = stackedButtonCenters(kMenuH, kBtnH, kBtnGap, 6);
        return {
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[0] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "1 PLAYER"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[1] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "2 PLAYER"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[2] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "HOST"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[3] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "JOIN"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[4] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "CONTROLS"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[5] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "QUIT"},
        };
    }
    if (state == MenuState::AI_DIFFICULTY) {
        auto ys = stackedButtonCenters(kMenuH, kBtnH, kBtnGap, 3);
        return {
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[0] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "EASY"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[1] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "MEDIUM"},
            {{{kMenuW / 2.f - kBtnW / 2.f, ys[2] - kBtnH / 2.f}, {kBtnW, kBtnH}}, "HARD"},
            {backRect, "BACK"},
        };
    }
    if (state == MenuState::HOST_WAITING || state == MenuState::JOIN_INPUT) {
        return {{backRect, "BACK"}};
    }
    // READY_TO_START
    return {{{{kMenuW / 2.f - kBtnW / 2.f, kMenuH / 2.f - kBtnH / 2.f}, {kBtnW, kBtnH}}, "START"}};
}

// Settings screen: 12 specs in fixed order.
// [0-4]  P1 UP/DOWN/LEFT/RIGHT/ATTACK (left column, x-centre=256)
// [5-9]  P2 UP/DOWN/LEFT/RIGHT/ATTACK (right column, x-centre=768)
// [10]   RESET TO DEFAULTS (centered)
// [11]   BACK (top-left)
inline std::vector<ButtonSpec> settingsButtonSpecs(const KeyBindings& b) {
    const sf::FloatRect backRect{{20.f, 20.f}, {120.f, 38.f}};
    const float rowY0 = 120.f;
    const float rowStep = kSettingsRowH + kSettingsRowGap;
    const float p1x = 256.f - kSettingsRowW / 2.f;
    const float p2x = 768.f - kSettingsRowW / 2.f;

    struct Row {
        const char* action;
        sf::Keyboard::Key key;
    };
    const Row p1rows[5] = {
        {"UP", b.p1.up},       {"DOWN", b.p1.down},     {"LEFT", b.p1.left},
        {"RIGHT", b.p1.right}, {"ATTACK", b.p1.attack},
    };
    const Row p2rows[5] = {
        {"UP", b.p2.up},       {"DOWN", b.p2.down},     {"LEFT", b.p2.left},
        {"RIGHT", b.p2.right}, {"ATTACK", b.p2.attack},
    };

    std::vector<ButtonSpec> specs;
    specs.reserve(12);

    for (int i = 0; i < 5; ++i) {
        std::string label =
            std::string(p1rows[i].action) + "  [" + nameFromKey(p1rows[i].key) + "]";
        specs.push_back(
            {{{p1x, rowY0 + static_cast<float>(i) * rowStep}, {kSettingsRowW, kSettingsRowH}},
             label});
    }
    for (int i = 0; i < 5; ++i) {
        std::string label =
            std::string(p2rows[i].action) + "  [" + nameFromKey(p2rows[i].key) + "]";
        specs.push_back(
            {{{p2x, rowY0 + static_cast<float>(i) * rowStep}, {kSettingsRowW, kSettingsRowH}},
             label});
    }
    // RESET
    specs.push_back({{{kMenuW / 2.f - kBtnW / 2.f, 400.f}, {kBtnW, kBtnH}}, "RESET TO DEFAULTS"});
    // BACK
    specs.push_back({backRect, "BACK"});

    return specs;
}

// Returns the x-centre and y position for the "PLAYER 1" / "PLAYER 2" column headers.
// col 0 = P1 (x=256), col 1 = P2 (x=768), y = 90.
inline sf::Vector2f settingsColumnHeaderPos(int col) { return {col == 0 ? 256.f : 768.f, 90.f}; }

// Semi-transparent backing panel for HOST_WAITING and JOIN_INPUT text.
// Spans y=78–498 on a 576-high canvas, covering all text positions.
inline sf::FloatRect menuInfoPanelRect() {
    return {{(kMenuW - 560.f) / 2.f, (kMenuH - 420.f) / 2.f}, {560.f, 420.f}};
}

// Pause button geometry (game window: kGameW x kGameH from game_canvas.h).
// index 0 = Resume, index 1 = Quit to Menu.
inline sf::FloatRect pauseButtonRect(int index) {
    constexpr float w = 220.f, h = 55.f, gap = 16.f;
    float top = kGameH / 2.f - h - gap / 2.f + static_cast<float>(index) * (h + gap);
    return {{kGameW / 2.f - w / 2.f, top}, {w, h}};
}
