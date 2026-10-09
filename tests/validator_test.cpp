// validator_test.cpp - checks the level rules: empty columns, loose tiles, spawn and checkpoints.

#include <print>
#include "check.hpp"
#include "map/Map.hpp"
#include "validation/Validator.hpp"

using namespace ljn;

//builds an empty map for a test
static Map makeMap(int width = 16, int height = 10) {
    return Map::create(width, height).value();
}

//adds a tile type with the given pixel size and returns its id
static TileId addTile(Map& map, int pixelWidth, int pixelHeight, bool solid = true) {
    const TileDef def{.file = "tiles/test.png",
                      .pixelWidth = pixelWidth,
                      .pixelHeight = pixelHeight,
                      .solid = solid};
    return map.addTileDef(def).value();
}

//puts a row of one-cell tiles along the bottom of the map, skipping one column if asked
static void fillFloor(Map& map, TileId id, int skipX = -1) {
    for (int x = 0; x < map.width(); ++x) {
        if (x != skipX) {
            CHECK(map.place({x, map.height() - 1}, id).has_value());
        }
    }
}

//an empty map has no spawn and every column is empty
static void testEmptyMap() {
    const Map map = makeMap();
    const auto report = validate(map);

    CHECK(report.hasErrors());
    CHECK(report.count(IssueCode::MissingSpawn) == 1);
    CHECK(report.count(IssueCode::EmptyColumn) == 16);
}

//a map with a floor, a spawn and a checkpoint has no problems at all
static void testValidMap() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    fillFloor(map, ground);
    CHECK(map.setSpawn({2, 8}));
    CHECK(map.addCheckpoint({10, 8}));

    const auto report = validate(map);
    CHECK(report.issues.empty());
    CHECK(!report.hasErrors());
}

//groups of connected cells need at least 3 cells, or they count as mistakes
static void testStrayTiles() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    const TileId wide2 = addTile(map, 64, 32);
    const TileId wide3 = addTile(map, 96, 32);
    fillFloor(map, ground);
    CHECK(map.setSpawn({2, 8}));

    //one loose cell
    CHECK(map.place({5, 4}, ground).has_value());
    CHECK(validate(map).count(IssueCode::StrayTiles) == 1);
    CHECK(map.erase({5, 4}).has_value());

    //a tile with 2 cells
    CHECK(map.place({5, 4}, wide2).has_value());
    CHECK(validate(map).count(IssueCode::StrayTiles) == 2);
    CHECK(map.erase({5, 4}).has_value());

    //a platform with 3 cells is fine
    CHECK(map.place({5, 4}, wide3).has_value());
    CHECK(validate(map).count(IssueCode::StrayTiles) == 0);
}

//a block touching the floor is part of the floor, so it is not loose
static void testBlockOnFloor() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    fillFloor(map, ground);
    CHECK(map.setSpawn({2, 8}));
    CHECK(map.place({5, 8}, ground).has_value());

    CHECK(validate(map).count(IssueCode::StrayTiles) == 0);
}

//a big tile counts all the cells it covers, and the minimum can be changed
static void testBigTileAndRules() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    const TileId big = addTile(map, 128, 128);
    fillFloor(map, ground);
    CHECK(map.setSpawn({2, 8}));
    CHECK(map.place({4, 2}, big).has_value());

    //16 cells is enough with the default minimum of 3
    CHECK(validate(map).count(IssueCode::StrayTiles) == 0);

    //with a minimum of 17 the same tile becomes a mistake, one issue per cell
    const auto strict = validate(map, Rules{.minStructureCells = 17});
    CHECK(strict.count(IssueCode::StrayTiles) == 16);
}

//the spawn must exist, must not be inside a block and needs ground below
static void testSpawn() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    fillFloor(map, ground);

    //no spawn
    CHECK(validate(map).count(IssueCode::MissingSpawn) == 1);

    //spawn inside the floor
    CHECK(map.setSpawn({3, 9}));
    CHECK(validate(map).count(IssueCode::SpawnInsideSolid) == 1);

    //spawn in the air above the floor is fine
    CHECK(map.setSpawn({3, 3}));
    CHECK(validate(map).issues.empty());
}

//a spawn over a pit has no ground below
static void testSpawnOverPit() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);

    //floor only on the left half of the map
    for (int x = 0; x < 8; ++x) {
        CHECK(map.place({x, 9}, ground).has_value());
    }
    CHECK(map.setSpawn({10, 5}));

    const auto report = validate(map);
    CHECK(report.count(IssueCode::SpawnNoGround) == 1);
    CHECK(report.count(IssueCode::EmptyColumn) == 8);
}

//a checkpoint inside a block is an error
static void testCheckpointInsideSolid() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    fillFloor(map, ground);
    CHECK(map.setSpawn({2, 8}));
    CHECK(map.addCheckpoint({10, 9}));

    const auto report = validate(map);
    CHECK(report.count(IssueCode::CheckpointInsideSolid) == 1);
    CHECK(report.hasErrors());
}

//a checkpoint with no ground below is only a warning
static void testCheckpointNoGround() {
    Map map = makeMap();
    const TileId ground = addTile(map, 32, 32);
    const TileId decor = addTile(map, 32, 32, false);

    //column 10 only has a non solid tile, so it is not empty but has no ground
    fillFloor(map, ground, 10);
    CHECK(map.place({10, 9}, decor).has_value());
    CHECK(map.setSpawn({2, 8}));
    CHECK(map.addCheckpoint({10, 5}));

    const auto report = validate(map);
    CHECK(report.issues.size() == 1);
    CHECK(report.issues[0].code == IssueCode::CheckpointNoGround);
    CHECK(report.issues[0].severity == Severity::Warning);
    CHECK(!report.hasErrors());
}

//every issue code has a text for the editor
static void testDescribe() {
    for (int i = 0; i <= static_cast<int>(IssueCode::CheckpointNoGround); ++i) {
        CHECK(!describe(static_cast<IssueCode>(i)).empty());
    }
}

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testEmptyMap();
    testValidMap();
    testStrayTiles();
    testBlockOnFloor();
    testBigTileAndRules();
    testSpawn();
    testSpawnOverPit();
    testCheckpointInsideSolid();
    testCheckpointNoGround();
    testDescribe();
    std::println("all validator tests passed");
}
