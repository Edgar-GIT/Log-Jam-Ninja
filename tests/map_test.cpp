// map_test.cpp - checks that Map creates, places, erases and looks up tiles correctly.

// Standard library headers used below.
#include <cstdio>
#include <cstdlib>
#include <print>

// The code under test.
#include "map/Map.hpp"

using namespace ljn;

// Stops the program with a message if the condition is false.
#define CHECK(...)                                                                    \
    do {                                                                              \
        if (!(__VA_ARGS__)) {                                                         \
            std::println(stderr, "FAIL {}:{}  {}", __FILE__, __LINE__, #__VA_ARGS__); \
            std::exit(1);                                                             \
        }                                                                             \
    } while (false)

// True if the result is an error and it is exactly the expected one.
static bool failsWith(const auto& result, auto error) {
    return !result.has_value() && result.error() == error;
}

// Builds an empty map for a test.
static Map makeMap(int width = 16, int height = 10) {
    return Map::create(width, height).value();
}

// Adds a tile type with the given pixel size and returns its ID.
static TileId addTile(Map& map, int pixelWidth, int pixelHeight, bool solid = true) {
    const TileDef def{.file = "tiles/test.png",
                      .pixelWidth = pixelWidth,
                      .pixelHeight = pixelHeight,
                      .solid = solid};
    return map.addTileDef(def).value();
}

// Valid sizes work; zero, negative and too-large sizes fail.
static void testCreate() {
    CHECK(Map::create(16, 10).has_value());
    CHECK(Map::create(16, 10)->width() == 16);
    CHECK(failsWith(Map::create(0, 10), MapError::InvalidSize));
    CHECK(failsWith(Map::create(16, -1), MapError::InvalidSize));
    CHECK(failsWith(Map::create(Map::kMaxWidth + 1, 10), MapError::InvalidSize));
}

// The palette hands out IDs 1, 2, 3... and rejects bad sprite sizes.
static void testPalette() {
    Map map = makeMap();
    CHECK(!map.hasTile(0));

    const TileId tileSmall = addTile(map, 32, 32);
    const TileId tileBig = addTile(map, 128, 128);
    const TileId tileOdd = addTile(map, 100, 40);
    CHECK(tileSmall == 1 && tileBig == 2 && tileOdd == 3);
    CHECK(map.hasTile(tileBig));
    CHECK(!map.hasTile(4));

    // Cell counts are rounded up: 100 px -> 4 cells, 40 px -> 2 cells.
    CHECK(map.tileDefs()[1].cellsWide() == 4 && map.tileDefs()[1].cellsHigh() == 4);
    CHECK(map.tileDefs()[2].cellsWide() == 4 && map.tileDefs()[2].cellsHigh() == 2);

    const TileDef zero{.file = "x.png", .pixelWidth = 0, .pixelHeight = 32};
    CHECK(failsWith(map.addTileDef(zero), MapError::InvalidTileSize));

    const TileDef huge{.file = "x.png",
                       .pixelWidth = (Map::kMaxTileCells + 1) * kCellSize,
                       .pixelHeight = 32};
    CHECK(failsWith(map.addTileDef(huge), MapError::InvalidTileSize));
}

// A one-cell tile can be placed and erased.
static void testSingleTile() {
    Map map = makeMap();
    const TileId id = addTile(map, 32, 32);

    CHECK(map.isFree({3, 4}));
    CHECK(map.place({3, 4}, id).has_value());
    CHECK(map.tile({3, 4}) == id);
    CHECK(!map.isFree({3, 4}));
    CHECK(map.isFree({4, 4}));

    const auto erased = map.erase({3, 4});
    CHECK(erased.has_value());
    CHECK(erased->origin == Cell{3, 4} && erased->id == id);
    CHECK(map.isFree({3, 4}));
    CHECK(!map.erase({3, 4}).has_value());
}

// A 128x128 tile covers 4x4 cells and is erased as a whole.
static void testBigTile() {
    Map map = makeMap();
    const TileId id = addTile(map, 128, 128);

    CHECK(map.place({2, 3}, id).has_value());
    for (int y = 3; y < 7; ++y) {
        for (int x = 2; x < 6; ++x) {
            CHECK(map.tile({x, y}) == id);
        }
    }
    CHECK(map.isFree({6, 3}));
    CHECK(map.isFree({2, 7}));
    CHECK(map.originOf({5, 6}) == Cell{2, 3});
    CHECK(map.originOf({2, 3}) == Cell{2, 3});

    // Erasing from the middle of the tile removes the whole tile.
    const auto erased = map.erase({4, 5});
    CHECK(erased.has_value() && erased->origin == Cell{2, 3} && erased->id == id);
    for (int y = 3; y < 7; ++y) {
        for (int x = 2; x < 6; ++x) {
            CHECK(map.isFree({x, y}));
        }
    }
}

// Placing fails with the right error and never changes the map when it fails.
static void testPlaceErrors() {
    Map map = makeMap();  // 16 x 10 cells
    const TileId tileSmall = addTile(map, 32, 32);
    const TileId tileBig = addTile(map, 128, 128);

    // IDs that are not in the palette.
    CHECK(failsWith(map.place({0, 0}, 0), PlaceError::UnknownTile));
    CHECK(failsWith(map.place({0, 0}, 99), PlaceError::UnknownTile));

    // Outside the map: negative origin, past the right edge, past the bottom edge.
    CHECK(failsWith(map.place({-1, 0}, tileSmall), PlaceError::OutOfBounds));
    CHECK(failsWith(map.place({14, 0}, tileBig), PlaceError::OutOfBounds));
    CHECK(failsWith(map.place({0, 8}, tileBig), PlaceError::OutOfBounds));
    CHECK(!map.canPlace({14, 0}, tileBig));

    // A 4x4 tile fits exactly in the bottom-right corner.
    CHECK(map.canPlace({12, 6}, tileBig));

    // Overlapping another tile.
    CHECK(map.place({3, 3}, tileSmall).has_value());
    CHECK(failsWith(map.place({1, 1}, tileBig), PlaceError::Overlap));
    CHECK(!map.canPlace({1, 1}, tileBig));
    CHECK(map.isFree({1, 1}));
    CHECK(map.tile({3, 3}) == tileSmall);
}

// Solidity follows the tile type, for every cell the tile covers.
static void testSolid() {
    Map map = makeMap();
    const TileId wall = addTile(map, 64, 32, true);
    const TileId decor = addTile(map, 32, 32, false);

    CHECK(map.place({1, 1}, wall).has_value());
    CHECK(map.place({5, 5}, decor).has_value());

    CHECK(map.isSolid({1, 1}));
    CHECK(map.isSolid({2, 1}));
    CHECK(!map.isSolid({5, 5}));
    CHECK(!map.isSolid({0, 0}));
    CHECK(!map.isSolid({-1, 0}));
    CHECK(!map.isSolid({99, 99}));
}

// Spawn must be inside the map; checkpoints stay sorted and without duplicates.
static void testSpawnAndCheckpoints() {
    Map map = makeMap();

    CHECK(!map.hasSpawn());
    CHECK(map.setSpawn({2, 2}));
    CHECK(map.hasSpawn() && map.spawn() == Cell{2, 2});
    CHECK(!map.setSpawn({16, 0}));
    CHECK(map.spawn() == Cell{2, 2});
    map.clearSpawn();
    CHECK(!map.hasSpawn());

    CHECK(map.addCheckpoint({8, 2}));
    CHECK(map.addCheckpoint({3, 5}));
    CHECK(map.addCheckpoint({3, 1}));
    CHECK(!map.addCheckpoint({3, 5}));
    CHECK(!map.addCheckpoint({-1, 0}));

    const auto list = map.checkpoints();
    CHECK(list.size() == 3);
    CHECK(list[0] == Cell{3, 1} && list[1] == Cell{3, 5} && list[2] == Cell{8, 2});

    CHECK(map.removeCheckpoint({3, 5}));
    CHECK(!map.removeCheckpoint({3, 5}));
    CHECK(map.checkpoints().size() == 2);
}

// Runs every test; any failed CHECK ends the program with exit code 1.
int main() {
    testCreate();
    testPalette();
    testSingleTile();
    testBigTile();
    testPlaceErrors();
    testSolid();
    testSpawnAndCheckpoints();
    std::println("all map tests passed");
}
