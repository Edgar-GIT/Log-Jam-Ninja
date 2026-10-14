// animation_test.cpp - checks which frame an animated tile shows, and the natural order of file names.

#include <print>
#include <string>
#include <vector>
#include "check.hpp"
#include "map/Animation.hpp"
#include "map/Map.hpp"
#include "util/NaturalSort.hpp"

using namespace ljn;

//a tile with three frames that change every 100 ms
static TileDef makeAnimated() {
    return {.file = "t/0.png",
            .frames = {"t/0.png", "t/1.png", "t/2.png"},
            .frameMs = 100};
}

//a tile that does not move always shows its only image
static void testStillTile() {
    const TileDef still{.file = "still.png"};
    CHECK(!still.isAnimated());
    CHECK(frameAt(still, 0.0) == 0);
    CHECK(frameAt(still, 12.5) == 0);
    CHECK(currentFile(still, 3.0) == "still.png");
}

//the frame changes every interval and starts again after the last one
static void testFrames() {
    const TileDef tile = makeAnimated();
    CHECK(tile.isAnimated());
    CHECK(frameAt(tile, 0.0) == 0);
    CHECK(frameAt(tile, 0.099) == 0);
    CHECK(frameAt(tile, 0.100) == 1);
    CHECK(frameAt(tile, 0.250) == 2);
    CHECK(frameAt(tile, 0.300) == 0);
    CHECK(frameAt(tile, 3600.0) == 0);
    CHECK(currentFile(tile, 0.15) == "t/1.png");
}

//a broken animation (one frame, or no speed) is drawn as a still tile instead of crashing
static void testBrokenAnimations() {
    TileDef oneFrame = makeAnimated();
    oneFrame.frames.resize(1);
    CHECK(!oneFrame.isAnimated());
    CHECK(frameAt(oneFrame, 5.0) == 0);

    TileDef noSpeed = makeAnimated();
    noSpeed.frameMs = 0;
    CHECK(!noSpeed.isAnimated());
    CHECK(frameAt(noSpeed, 5.0) == 0);
    CHECK(currentFile(noSpeed, 5.0) == "t/0.png");
}

//numbers inside names count as numbers
static void testNaturalOrder() {
    CHECK(naturalLess("frame2", "frame10"));
    CHECK(!naturalLess("frame10", "frame2"));
    CHECK(naturalLess("9", "10"));
    CHECK(naturalLess("a", "a1"));
    CHECK(naturalLess("a2", "b1"));
    CHECK(naturalLess("torch_1.png", "torch_2.png"));

    //the same number written with zeros is not before or after
    CHECK(!naturalLess("a01", "a1"));
    CHECK(!naturalLess("a1", "a01"));

    std::vector<std::string> names{"f10", "f2", "f1", "f20", "f3"};
    std::ranges::sort(names, [](const std::string& a, const std::string& b) { return naturalLess(a, b); });
    CHECK((names == std::vector<std::string>{"f1", "f2", "f3", "f10", "f20"}));
}

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testStillTile();
    testFrames();
    testBrokenAnimations();
    testNaturalOrder();
    std::println("all animation tests passed");
}
