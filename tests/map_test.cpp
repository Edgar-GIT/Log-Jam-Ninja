// map_test.cpp - checks that Map creates, places, erases and looks up tiles correctly.

#include <print>
#include <string>
#include "check.hpp"
#include "map/Map.hpp"

using namespace ljn;

//creates a fake map with one solid layer (index 0)
static Map makeMap(int width = 16, int height = 10) {
    Map map = Map::create(width, height).value();
    CHECK(map.addLayer({.name = "Ground", .solid = true}).has_value());
    return map;
}

//adds a tile type with the given pixel size and returns its ID
static TileId addTile(Map& map, int pixelWidth, int pixelHeight, bool solid = true) {
    const TileDef def{.file = "tiles/test.png",
                      .pixelWidth = pixelWidth,
                      .pixelHeight = pixelHeight,
                      .solid = solid};
    return map.addTileDef(def).value();
}

//testing sprite sizes; zero, negative and too-large sizes fail
static void testCreate() {
    CHECK(Map::create(16, 10).has_value());
    CHECK(Map::create(16, 10)->width() == 16);
    CHECK(failsWith(Map::create(0, 10), MapError::InvalidSize));
    CHECK(failsWith(Map::create(16, -1), MapError::InvalidSize));
    CHECK(failsWith(Map::create(Map::kMaxWidth + 1, 10), MapError::InvalidSize));
}

//the palette hands out IDs and rejects bad sprite sizes
static void testPalette() {
    Map map = makeMap();
    CHECK(!map.hasTile(0));

    const TileId tileSmall = addTile(map, 32, 32);
    const TileId tileBig = addTile(map, 128, 128);
    const TileId tileOdd = addTile(map, 100, 40);
    CHECK(tileSmall == 1 && tileBig == 2 && tileOdd == 3);
    CHECK(map.hasTile(tileBig));
    CHECK(!map.hasTile(4));

    //cell counts are rounded up, 100 px -> 4 cells, 40 px -> 2 cells
    CHECK(map.tileDefs()[1].cellsWide() == 4 && map.tileDefs()[1].cellsHigh() == 4);
    CHECK(map.tileDefs()[2].cellsWide() == 4 && map.tileDefs()[2].cellsHigh() == 2);

    const TileDef zero{.file = "x.png", .pixelWidth = 0, .pixelHeight = 32};
    CHECK(failsWith(map.addTileDef(zero), MapError::InvalidTileSize));

    const TileDef huge{.file = "x.png",
                       .pixelWidth = (Map::kMaxTileCells + 1) * kCellSize,
                       .pixelHeight = 32};
    CHECK(failsWith(map.addTileDef(huge), MapError::InvalidTileSize));
}

//aone-cell tile can be placed and erased
static void testSingleTile() {
    Map map = makeMap();
    const TileId id = addTile(map, 32, 32);

    CHECK(map.isFree(0, {3, 4}));
    CHECK(map.place(0, {3, 4}, id).has_value());
    CHECK(map.tile(0, {3, 4}) == id);
    CHECK(!map.isFree(0, {3, 4}));
    CHECK(map.isFree(0, {4, 4}));

    const auto erased = map.erase(0, {3, 4});
    CHECK(erased.has_value());
    CHECK(erased->origin == Cell{3, 4} && erased->id == id);
    CHECK(map.isFree(0, {3, 4}));
    CHECK(!map.erase(0, {3, 4}).has_value());
}

//a 128x128 tile covers 4x4 cells and is erased as a whole
static void testBigTile() {
    Map map = makeMap();
    const TileId id = addTile(map, 128, 128);

    CHECK(map.place(0, {2, 3}, id).has_value());
    for (int y = 3; y < 7; ++y) {
        for (int x = 2; x < 6; ++x) {
            CHECK(map.tile(0, {x, y}) == id);
        }
    }
    CHECK(map.isFree(0, {6, 3}));
    CHECK(map.isFree(0, {2, 7}));
    CHECK(map.originOf(0, {5, 6}) == Cell{2, 3});
    CHECK(map.originOf(0, {2, 3}) == Cell{2, 3});

    //erasing from the middle of the tile removes the whole tile
    const auto erased = map.erase(0, {4, 5});
    CHECK(erased.has_value() && erased->origin == Cell{2, 3} && erased->id == id);
    for (int y = 3; y < 7; ++y) {
        for (int x = 2; x < 6; ++x) {
            CHECK(map.isFree(0, {x, y}));
        }
    }
}

//placing fails with the right error and never changes the map when it fails
static void testPlaceErrors() {
    Map map = makeMap();  // 16 x 10 cells
    const TileId tileSmall = addTile(map, 32, 32);
    const TileId tileBig = addTile(map, 128, 128);

    //IDs that are not in the palette
    CHECK(failsWith(map.place(0, {0, 0}, 0), PlaceError::UnknownTile));
    CHECK(failsWith(map.place(0, {0, 0}, 99), PlaceError::UnknownTile));

    //outside the map: negative origin, past the right edge, past the bottom edge
    CHECK(failsWith(map.place(0, {-1, 0}, tileSmall), PlaceError::OutOfBounds));
    CHECK(failsWith(map.place(0, {14, 0}, tileBig), PlaceError::OutOfBounds));
    CHECK(failsWith(map.place(0, {0, 8}, tileBig), PlaceError::OutOfBounds));
    CHECK(!map.canPlace(0, {14, 0}, tileBig));

    //a 4x4 tile fits exactly in the bottom-right corner
    CHECK(map.canPlace(0, {12, 6}, tileBig));

    //overlapping another tile
    CHECK(map.place(0, {3, 3}, tileSmall).has_value());
    CHECK(failsWith(map.place(0, {1, 1}, tileBig), PlaceError::Overlap));
    CHECK(!map.canPlace(0, {1, 1}, tileBig));
    CHECK(map.isFree(0, {1, 1}));
    CHECK(map.tile(0, {3, 3}) == tileSmall);
}

//solidity follows the tile type for every cell the tile covers
static void testSolid() {
    Map map = makeMap();
    const TileId wall = addTile(map, 64, 32, true);
    const TileId decor = addTile(map, 32, 32, false);

    CHECK(map.place(0, {1, 1}, wall).has_value());
    CHECK(map.place(0, {5, 5}, decor).has_value());

    CHECK(map.isSolid({1, 1}));
    CHECK(map.isSolid({2, 1}));
    CHECK(!map.isSolid({5, 5}));
    CHECK(!map.isSolid({0, 0}));
    CHECK(!map.isSolid({-1, 0}));
    CHECK(!map.isSolid({99, 99}));
}

//spawn must be inside the map and  checkpoints stay sorted and without duplicates
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

//layers: add, limits, names, move, remove, settings
static void testLayers() {
    Map map = Map::create(8, 6).value();
    CHECK(map.layerCount() == 0);
    CHECK(!map.hasLayer(0));

    //layers are added in front, so the first one added is the back one
    CHECK(map.addLayer({.name = "Back", .parallaxX = 0.5f}).value() == 0);
    CHECK(map.addLayer({.name = "Ground", .solid = true}).value() == 1);
    CHECK(map.addLayer({.name = "Front"}).value() == 2);
    CHECK(map.layerCount() == 3);
    CHECK(map.layerInfo(0).parallaxX == 0.5f && map.layerInfo(0).parallaxY == 1.0f);
    CHECK(map.layerInfo(1).solid && !map.layerInfo(0).solid);

    //bad names are refused
    CHECK(failsWith(map.addLayer({.name = ""}), MapError::InvalidLayer));
    CHECK(failsWith(map.addLayer({.name = std::string(Map::kMaxLayerName + 1, 'a')}), MapError::InvalidLayer));
    CHECK(!map.setLayerInfo(0, {.name = ""}));
    CHECK(!map.setLayerInfo(9, {.name = "x"}));

    //settings change
    CHECK(map.setLayerInfo(2, {.name = "Fog", .parallaxX = 2.0f, .visible = false}));
    CHECK(map.layerInfo(2).name == "Fog" && !map.layerInfo(2).visible);

    //move: Back, Ground, Fog -> Ground, Fog, Back and then back again
    CHECK(map.moveLayer(0, 2));
    CHECK(map.layerInfo(0).name == "Ground" && map.layerInfo(1).name == "Fog" && map.layerInfo(2).name == "Back");
    CHECK(map.moveLayer(2, 0));
    CHECK(map.layerInfo(0).name == "Back" && map.layerInfo(1).name == "Ground" && map.layerInfo(2).name == "Fog");
    CHECK(!map.moveLayer(0, 3));
    CHECK(!map.moveLayer(-1, 0));

    //remove gives the layer back with its tiles, and insert puts it in the same place
    const TileId id = addTile(map, 32, 32);
    CHECK(map.place(1, {2, 2}, id).has_value());
    auto removed = map.removeLayer(1);
    CHECK(removed.has_value() && removed->info.name == "Ground");
    CHECK(map.layerCount() == 2 && !map.removeLayer(5).has_value());
    CHECK(map.tile(1, {2, 2}) == kEmptyTile);
    CHECK(map.insertLayer(1, std::move(*removed)).has_value());
    CHECK(map.tile(1, {2, 2}) == id);

    //a layer with the wrong size cannot be inserted
    CHECK(failsWith(map.insertLayer(0, Map::Layer{{.name = "Bad"}, {}}), MapError::InvalidLayer));

    //there is a limit
    while (map.layerCount() < Map::kMaxLayers) {
        CHECK(map.addLayer({.name = "More"}).has_value());
    }
    CHECK(failsWith(map.addLayer({.name = "One too many"}), MapError::TooManyLayers));
}

//every layer has its own tiles, and only solid layers block movement
static void testLayerContents() {
    Map map = Map::create(8, 6).value();
    CHECK(map.addLayer({.name = "Back"}).has_value());
    CHECK(map.addLayer({.name = "Ground", .solid = true}).has_value());
    const TileId wall = addTile(map, 32, 32, true);

    //the same cell can hold a tile in each layer
    CHECK(map.place(0, {3, 3}, wall).has_value());
    CHECK(map.place(1, {3, 3}, wall).has_value());
    CHECK(failsWith(map.place(1, {3, 3}, wall), PlaceError::Overlap));
    CHECK(failsWith(map.place(7, {3, 3}, wall), PlaceError::UnknownLayer));
    CHECK(map.hasAnyTile({3, 3}) && !map.hasAnyTile({4, 3}));

    //a solid tile on a layer that is not solid does not block
    CHECK(map.place(0, {5, 5}, wall).has_value());
    CHECK(!map.isSolid({5, 5}));
    CHECK(map.isSolid({3, 3}));

    //erasing in one layer leaves the other alone
    CHECK(map.erase(1, {3, 3}).has_value());
    CHECK(map.tile(0, {3, 3}) == wall && map.tile(1, {3, 3}) == kEmptyTile);
    CHECK(!map.isSolid({3, 3}));

    //making the layer solid changes the answer
    CHECK(map.setLayerInfo(0, {.name = "Back", .solid = true}));
    CHECK(map.isSolid({3, 3}) && map.isSolid({5, 5}));
}

//runs every test and any failed CHECK ends the program with exit code 1
int main() {
    testCreate();
    testPalette();
    testSingleTile();
    testBigTile();
    testPlaceErrors();
    testSolid();
    testSpawnAndCheckpoints();
    testLayers();
    testLayerContents();
    std::println("all map tests passed");
}
