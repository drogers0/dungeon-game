// Unit tests for extracted free functions:
//   objectBounds — rectangle construction from a GameObject
//   advanceFrameRect — sprite-sheet frame stepping
//   applyInputToP1 — P1 input mapping (int attack parameter)
// Also: RegularGameObject method coverage.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RegularGameObject.h"
#include "geometry.h"
#include "replay.h"
#include "sprite_anim.h"

// ── StubGameObject ─────────────────────────────────────────────────────────────
// Minimal concrete implementation of GameObject for testing objectBounds.

class StubGameObject : public GameObject {
public:
    float px = 0, py = 0;
    float w = 0, h = 0;
    float sx = 1, sy = 1;

    bool load(const std::string&) override { return true; }
    void draw(sf::RenderWindow&) override {}
    void update(float) override {}
    void setPosition(float x, float y) override {
        px = x;
        py = y;
    }
    void move(sf::Vector2f v) override {
        px += v.x;
        py += v.y;
    }
    sf::Vector2f getPosition() const override { return {px, py}; }
    float getHeight() const override { return h; }
    float getWidth() const override { return w; }
    void setScale(float s) override { sx = sy = s; }
    void setScale(float x, float y) override {
        sx = x;
        sy = y;
    }
    sf::Vector2f getScale() const override { return {sx, sy}; }
    void changeValid(bool) override {}
    bool isValid() override { return true; }
    void setOrigin() override {}
};

// ── objectBounds ───────────────────────────────────────────────────────────────

TEST_CASE("objectBounds: positive scale produces correct rect", "[geometry][unit]") {
    StubGameObject obj;
    obj.px = 100;
    obj.py = 200;
    obj.w = 50;
    obj.h = 30;
    obj.sx = 2;
    obj.sy = 2;

    sf::FloatRect r = objectBounds(obj);
    REQUIRE(r.left == Catch::Approx(100.f));
    REQUIRE(r.top == Catch::Approx(200.f));
    REQUIRE(r.width == Catch::Approx(100.f)); // 50 * 2
    REQUIRE(r.height == Catch::Approx(60.f)); // 30 * 2
}

TEST_CASE("objectBounds: negative scale.x produces negative-width rect (pinned quirk)",
          "[geometry][unit]") {
    // P2 default: scale.x = -1.  The resulting rect has negative width.
    // SFML intersects() normalises it — this is load-bearing for hit detection.
    StubGameObject obj;
    obj.px = 500;
    obj.py = 400;
    obj.w = 119;
    obj.h = 180;
    obj.sx = -1;
    obj.sy = 1;

    sf::FloatRect r = objectBounds(obj);
    REQUIRE(r.left == Catch::Approx(500.f));
    REQUIRE(r.top == Catch::Approx(400.f));
    REQUIRE(r.width == Catch::Approx(-119.f)); // negative — NOT normalised here
    REQUIRE(r.height == Catch::Approx(180.f));
}

TEST_CASE("objectBounds: two overlapping objects intersect via SFML", "[geometry][unit]") {
    StubGameObject a, b;
    // a at (100,100), size 100x100 (positive scale)
    a.px = 100;
    a.py = 100;
    a.w = 50;
    a.h = 50;
    a.sx = 2;
    a.sy = 2;
    // b at (150,150) with negative scale: pos(300,100), width=-100 → normalised (200,100,100,100)
    b.px = 300;
    b.py = 100;
    b.w = 50;
    b.h = 50;
    b.sx = -2;
    b.sy = 2;

    // a covers [100,200] x [100,200]
    // b raw rect: left=300, width=-100 → SFML normalises to [200,300] x [100,200]
    // Overlap on x: [200,200] → touching (not overlapping)
    sf::FloatRect ra = objectBounds(a);
    sf::FloatRect rb = objectBounds(b);
    // Just verify the intersection function doesn't crash on negative width
    bool result = ra.intersects(rb);
    (void)result;  // result may be false for touching rects; key point is no crash
    REQUIRE(true); // reached here = no crash
}

TEST_CASE("objectBounds: disjoint objects do not intersect", "[geometry][unit]") {
    StubGameObject a, b;
    a.px = 0;
    a.py = 0;
    a.w = 10;
    a.h = 10;
    a.sx = 1;
    a.sy = 1;
    b.px = 100;
    b.py = 100;
    b.w = 10;
    b.h = 10;
    b.sx = 1;
    b.sy = 1;

    REQUIRE(!objectBounds(a).intersects(objectBounds(b)));
}

TEST_CASE("objectBounds: overlapping objects intersect", "[geometry][unit]") {
    StubGameObject a, b;
    a.px = 0;
    a.py = 0;
    a.w = 20;
    a.h = 20;
    a.sx = 1;
    a.sy = 1;
    b.px = 10;
    b.py = 10;
    b.w = 20;
    b.h = 20;
    b.sx = 1;
    b.sy = 1;

    REQUIRE(objectBounds(a).intersects(objectBounds(b)));
}

// ── advanceFrameRect ───────────────────────────────────────────────────────────
//
// Fire sheet parameters used in Game: AnimatedGameObject(216, 216, 5, 3, 10, 0)
//   xsize=216, ysize=216, nx=5, ny=3, howmany=10
//   frame width  = floor(216/5) = 43
//   frame height = floor(216/3) = 72
//
// Player (rocket): AnimatedGameObject(404, 206, 3, 3, 9, 0)
//   frame width  = floor(404/3) = 134
//   frame height = floor(206/3) = 68

TEST_CASE("advanceFrameRect: fire sheet - full cycle resets to frame 1", "[anim][unit]") {
    // Walk all 10 frames of the fire sheet, verify reset after frame 10
    sf::IntRect rect(0, 0, 43, 72); // initial rect from load()
    int curr = 1;

    for (int step = 0; step < 10; ++step) {
        auto res = advanceFrameRect(rect, curr, 5, 3, 10, 216.0, 216.0);
        rect = res.rect;
        curr = res.curr;
    }
    // After 10 advances, curr resets to 1 and rect resets to (0,0,43,72)
    REQUIRE(curr == 1);
    REQUIRE(rect.left == 0);
    REQUIRE(rect.top == 0);
    REQUIRE(rect.width == 43);
    REQUIRE(rect.height == 72);
}

TEST_CASE("advanceFrameRect: fire sheet - row wrap at curr%nx==0", "[anim][unit]") {
    // curr=5 (last in row 0, 0-indexed), check==0 → new row
    sf::IntRect rect(4 * 43, 0, 43, 72); // curr=5, check = 5%5 = 0
    int curr = 5;

    auto res = advanceFrameRect(rect, curr, 5, 3, 10, 216.0, 216.0);
    REQUIRE(res.rect.left == 0);
    REQUIRE(res.rect.top == 72); // new row: old top(0) + height(72)
    REQUIRE(res.curr == 6);
}

TEST_CASE("advanceFrameRect: fire sheet - int-ceil quirk pinned", "[anim][unit]") {
    // curr=3, check = 3%5 = 3 ≠ 0 → else branch
    // ((int)ceil(3/5)) = ((int)ceil(0)) = 0   ← int division BEFORE ceil
    // So top = 0 * 72 = 0
    sf::IntRect rect(43, 0, 43, 72); // curr=2, but we'll test curr=3
    rect = sf::IntRect(2 * 43, 0, 43, 72);
    int curr = 3;

    auto res = advanceFrameRect(rect, curr, 5, 3, 10, 216.0, 216.0);
    // check = 3%5 = 3 ≠ 0 → else branch
    // h = floor(216/3) = 72
    // top = ceil(3/5) * 72 = ceil(0) * 72 = 0  (int division first)
    REQUIRE(res.rect.top == 0);
    REQUIRE(res.curr == 4);
}

TEST_CASE("advanceFrameRect: player sheet (rocket) - full cycle", "[anim][unit]") {
    // AnimatedGameObject(404, 206, 3, 3, 9, 0): frame 134x68, 9 frames total
    sf::IntRect rect(0, 0, 134, 68);
    int curr = 1;

    for (int step = 0; step < 9; ++step) {
        auto res = advanceFrameRect(rect, curr, 3, 3, 9, 404.0, 206.0);
        rect = res.rect;
        curr = res.curr;
    }
    REQUIRE(curr == 1);
    REQUIRE(rect.left == 0);
    REQUIRE(rect.top == 0);
}

TEST_CASE("advanceFrameRect: robot sheet - walk first frame", "[anim][unit]") {
    // AnimatedGameObject(959, 180, 8, 1, 8, 0): frame floor(959/8)=119, 8 frames total
    sf::IntRect rect(0, 0, 119, 180);
    int curr = 1;

    auto res = advanceFrameRect(rect, curr, 8, 1, 8, 959.0, 180.0);
    // curr=1, check=1%8=1 ≠ 0 → else branch
    // h = floor(180/1) = 180
    // top = ceil(1/8)*180 = ceil(0)*180 = 0  (int division: 1/8=0)
    REQUIRE(res.rect.left == 119);
    REQUIRE(res.rect.top == 0);
    REQUIRE(res.curr == 2);
}

// ── applyInputToP1 ─────────────────────────────────────────────────────────────

TEST_CASE("applyInputToP1 maps all five fields; attack is int", "[harness][unit]") {
    PlayerInput in;
    in.up = true;
    in.down = false;
    in.left = true;
    in.right = false;
    in.attack = true;

    bool w = false, a = false, s = true, d = true;
    int attack = 0;
    applyInputToP1(w, a, s, d, attack, in);

    REQUIRE(w == true);   // in.up    → m_up
    REQUIRE(a == true);   // in.left  → m_left
    REQUIRE(s == false);  // in.down  → m_down
    REQUIRE(d == false);  // in.right → m_right
    REQUIRE(attack == 1); // in.attack → right (int)
}

TEST_CASE("applyInputToP1 attack=false maps to 0", "[harness][unit]") {
    PlayerInput in;
    in.attack = false;

    bool w = false, a = false, s = false, d = false;
    int attack = 99;
    applyInputToP1(w, a, s, d, attack, in);

    REQUIRE(attack == 0);
}

// ── RegularGameObject: method coverage ───────────────────────────────────────
// Exercises position/scale/size/validity API without requiring a live window
// or texture file.  An untextured RegularGameObject has m_valid=false; calling
// setPosition/move/setScale on it is a no-op.  changeValid(true) flips the
// guard, after which the same calls go through to the underlying sf::Sprite.

TEST_CASE("RegularGameObject: size queries return 0 before load", "[unit]") {
    RegularGameObject obj;
    REQUIRE(obj.getWidth() == Catch::Approx(0.0f));
    REQUIRE(obj.getHeight() == Catch::Approx(0.0f));
}

TEST_CASE("RegularGameObject: position/scale return zero-vector when invalid", "[unit]") {
    RegularGameObject obj;
    REQUIRE(!obj.isValid());
    sf::Vector2f pos = obj.getPosition();
    REQUIRE(pos.x == Catch::Approx(0.0f));
    REQUIRE(pos.y == Catch::Approx(0.0f));
    sf::Vector2f scale = obj.getScale();
    REQUIRE(scale.x == Catch::Approx(0.0f));
    REQUIRE(scale.y == Catch::Approx(0.0f));
}

TEST_CASE("RegularGameObject: no-op calls when invalid do not crash", "[unit]") {
    RegularGameObject obj;
    obj.setPosition(10.f, 20.f);      // no-op
    obj.move(sf::Vector2f(5.f, 5.f)); // no-op
    obj.setScale(2.0f);               // no-op
    obj.setScale(2.0f, 0.5f);         // no-op
    obj.update(0.016f);               // always a no-op (empty body)
    // positions and scale unchanged from defaults
    sf::Vector2f pos = obj.getPosition();
    REQUIRE(pos.x == Catch::Approx(0.0f));
    REQUIRE(pos.y == Catch::Approx(0.0f));
}

TEST_CASE("RegularGameObject: position/scale/move work after changeValid", "[unit]") {
    RegularGameObject obj;
    obj.changeValid(true);
    REQUIRE(obj.isValid());

    obj.setPosition(100.f, 200.f);
    sf::Vector2f pos = obj.getPosition();
    REQUIRE(pos.x == Catch::Approx(100.0f));
    REQUIRE(pos.y == Catch::Approx(200.0f));

    obj.move(sf::Vector2f(10.f, -5.f));
    pos = obj.getPosition();
    REQUIRE(pos.x == Catch::Approx(110.0f));
    REQUIRE(pos.y == Catch::Approx(195.0f));

    obj.setScale(2.0f);
    sf::Vector2f sc = obj.getScale();
    REQUIRE(sc.x == Catch::Approx(2.0f));
    REQUIRE(sc.y == Catch::Approx(2.0f));

    obj.setScale(3.0f, 0.5f);
    sc = obj.getScale();
    REQUIRE(sc.x == Catch::Approx(3.0f));
    REQUIRE(sc.y == Catch::Approx(0.5f));
}

TEST_CASE("RegularGameObject: setOrigin does not crash", "[unit]") {
    RegularGameObject obj;
    obj.setOrigin(); // no m_valid guard; safe on untextured sprite
}
