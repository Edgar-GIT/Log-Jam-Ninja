// history_test.cpp - checks undo, redo and the saved marker of the edit history.

#include <cstddef>
#include <print>
#include <vector>
#include "check.hpp"
#include "map/History.hpp"
#include "map/Map.hpp"

using namespace ljn;

//everything that makes two maps different, as a list of numbers
static std::vector<int> fingerprint(const Map& map) {
    std::vector<int> out;
    out.push_back(map.layerCount());
    for (int layer = 0; layer < map.layerCount(); ++layer) {
        const LayerInfo& info = map.layerInfo(layer);
        out.push_back(static_cast<int>(info.name.size()));
        out.push_back(static_cast<int>(info.name[0]));
        out.push_back(static_cast<int>(info.parallaxX * 100.0f));
        out.push_back(info.solid ? 1 : 0);
        out.push_back(info.visible ? 1 : 0);

        const auto grid = map.grid(layer);
        for (std::size_t y = 0; y < grid.extent(0); ++y) {
            for (std::size_t x = 0; x < grid.extent(1); ++x) {
                out.push_back(grid[y, x].id);
                out.push_back(grid[y, x].dx);
                out.push_back(grid[y, x].dy);
            }
        }
    }
    out.push_back(map.hasSpawn() ? 1 : 0);
    if (map.hasSpawn()) {
        out.push_back(map.spawn().x);
        out.push_back(map.spawn().y);
    }
    for (const Cell& cell : map.checkpoints()) {
        out.push_back(cell.x);
        out.push_back(cell.y);
    }
    return out;
}

//a small map with a 1x1 tile type and a 4x4 tile type
struct Setup {
    Map map;
    TileId small;
    TileId big;
};

static Setup makeSetup() {
    Map map = Map::create(20, 10).value();
    CHECK(map.addLayer({.name = "Ground", .solid = true}).has_value());
    const TileId small = map.addTileDef({.file = "small.png"}).value();
    const TileId big = map.addTileDef({.file = "big.png", .pixelWidth = 128, .pixelHeight = 128}).value();
    return {std::move(map), small, big};
}

//place, undo and redo bring the map back and forth
static void testPlaceUndoRedo() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto empty = fingerprint(map);

    CHECK(history.place(map, 0, {3, 3}, small).has_value());
    history.commit();
    const auto placed = fingerprint(map);
    CHECK(placed != empty);

    const unsigned before = history.revision();
    CHECK(history.undo(map));
    CHECK(fingerprint(map) == empty);
    CHECK(history.revision() != before);
    CHECK(history.redo(map));
    CHECK(fingerprint(map) == placed);

    //nothing more to redo
    CHECK(!history.redo(map));
}

//erasing a big tile and undoing it brings back every cell of it
static void testEraseBigTile() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(history.place(map, 0, {4, 2}, big).has_value());
    history.commit();
    const auto withTile = fingerprint(map);

    //erase from a cell in the middle of the tile
    const auto removed = history.erase(map, 0, {6, 4});
    CHECK(removed.has_value() && removed->origin == Cell{4, 2});
    history.commit();
    CHECK(map.tile(0, {6, 4}) == kEmptyTile);

    CHECK(history.undo(map));
    CHECK(fingerprint(map) == withTile);
    CHECK(map.tile(0, {7, 5}) == big);
    CHECK(map.originOf(0, {7, 5}) == Cell{4, 2});
}

//edits made before one commit are a single action
static void testGroupedEdits() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto empty = fingerprint(map);

    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    CHECK(history.place(map, 0, {2, 1}, small).has_value());
    CHECK(history.place(map, 0, {3, 1}, small).has_value());
    history.commit();
    const auto painted = fingerprint(map);

    CHECK(history.undo(map));
    CHECK(fingerprint(map) == empty);
    CHECK(!history.canUndo());
    CHECK(history.redo(map));
    CHECK(fingerprint(map) == painted);
}

//undo also closes the group that is still open
static void testUndoWithoutCommit() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto empty = fingerprint(map);

    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    CHECK(history.canUndo());
    CHECK(history.undo(map));
    CHECK(fingerprint(map) == empty);
}

//edits that fail change nothing and are not remembered
static void testFailedEditsAreNotRecorded() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    history.commit();

    CHECK(!history.place(map, 0, {1, 1}, small).has_value());
    CHECK(!history.place(map, 0, {99, 1}, small).has_value());
    CHECK(!history.erase(map, 0, {9, 9}).has_value());
    CHECK(!history.setSpawn(map, {99, 99}));
    CHECK(!history.removeCheckpoint(map, {5, 5}));
    CHECK(!history.clearSpawn(map));
    history.commit();

    //only the first placement is in the history
    CHECK(history.undo(map));
    CHECK(map.tile(0, {1, 1}) == kEmptyTile);
    CHECK(!history.canUndo());
}

//the spawn point: set, move, remove and take it all back
static void testSpawn() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;

    CHECK(history.setSpawn(map, {2, 2}));
    history.commit();
    CHECK(history.setSpawn(map, {5, 5}));
    history.commit();

    //the same cell again is not an edit
    CHECK(!history.setSpawn(map, {5, 5}));

    CHECK(history.undo(map));
    CHECK(map.hasSpawn() && map.spawn() == Cell{2, 2});
    CHECK(history.undo(map));
    CHECK(!map.hasSpawn());
    CHECK(history.redo(map));
    CHECK(history.redo(map));
    CHECK(map.hasSpawn() && map.spawn() == Cell{5, 5});

    CHECK(history.clearSpawn(map));
    history.commit();
    CHECK(!map.hasSpawn());
    CHECK(history.undo(map));
    CHECK(map.hasSpawn() && map.spawn() == Cell{5, 5});
}

//checkpoints: add, remove and take them back
static void testCheckpoints() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;

    CHECK(history.addCheckpoint(map, {4, 4}));
    CHECK(history.addCheckpoint(map, {2, 2}));
    history.commit();
    CHECK(!history.addCheckpoint(map, {2, 2}));

    CHECK(history.removeCheckpoint(map, {4, 4}));
    history.commit();
    CHECK(map.checkpoints().size() == 1);

    CHECK(history.undo(map));
    CHECK(map.checkpoints().size() == 2);
    CHECK(map.checkpoints()[0] == Cell{2, 2} && map.checkpoints()[1] == Cell{4, 4});

    CHECK(history.undo(map));
    CHECK(map.checkpoints().empty());
}

//a new edit after an undo throws the redo list away
static void testNewEditClearsRedo() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;

    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    history.commit();
    CHECK(history.undo(map));
    CHECK(history.canRedo());

    CHECK(history.place(map, 0, {2, 2}, small).has_value());
    history.commit();
    CHECK(!history.canRedo());
}

//the dirty flag follows the saved state
static void testDirtyFlag() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(!history.isDirty());

    //an edit that is not committed yet already counts
    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    CHECK(history.isDirty());
    history.commit();
    CHECK(history.isDirty());

    history.markSaved();
    CHECK(!history.isDirty());

    CHECK(history.place(map, 0, {2, 1}, small).has_value());
    history.commit();
    CHECK(history.isDirty());
    CHECK(history.undo(map));
    CHECK(!history.isDirty());
    CHECK(history.redo(map));
    CHECK(history.isDirty());
    CHECK(history.undo(map));
    CHECK(!history.isDirty());

    //going back before the save and then editing makes a state that was never saved
    CHECK(history.undo(map));
    CHECK(history.isDirty());
    CHECK(history.place(map, 0, {3, 3}, small).has_value());
    history.commit();
    CHECK(history.isDirty());
}

//old actions are forgotten after the limit, and the saved state is lost with them
static void testLimit() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    history.markSaved();

    for (std::size_t i = 0; i < History::kMaxActions + 10; ++i) {
        CHECK(history.addCheckpoint(map, {1, 1}));
        history.commit();
        CHECK(history.removeCheckpoint(map, {1, 1}));
        history.commit();
    }

    std::size_t undone = 0;
    while (history.undo(map)) {
        ++undone;
    }
    CHECK(undone == History::kMaxActions);
    CHECK(history.isDirty());
}

//clear forgets everything and the map counts as saved
static void testClear() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    history.commit();
    CHECK(history.undo(map));

    history.clear();
    CHECK(!history.canUndo() && !history.canRedo() && !history.isDirty());
}

//runs every test, any failed CHECK ends the program with exit code 1
//layer edits are undone and redone like everything else, and each one is its own action
static void testLayerEdits() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto start = fingerprint(map);

    //add a layer, put a tile in it, then edit its settings
    const auto added = history.addLayer(map, {.name = "Sky", .parallaxX = 0.5f});
    CHECK(added.has_value() && *added == 1);
    CHECK(history.place(map, 1, {2, 2}, small).has_value());
    history.commit();
    CHECK(history.editLayer(map, 1, {.name = "Clouds", .parallaxX = 0.25f, .solid = true}));
    CHECK(map.layerInfo(1).name == "Clouds");
    const auto edited = fingerprint(map);

    //editing with the same settings does nothing
    CHECK(!history.editLayer(map, 1, map.layerInfo(1)));

    //three undos: the edit, the tile and the layer
    CHECK(history.undo(map));
    CHECK(map.layerInfo(1).name == "Sky" && map.layerInfo(1).parallaxX == 0.5f);
    CHECK(history.undo(map));
    CHECK(map.tile(1, {2, 2}) == kEmptyTile);
    CHECK(history.undo(map));
    CHECK(map.layerCount() == 1);
    CHECK(fingerprint(map) == start);

    //and three redos bring everything back
    CHECK(history.redo(map) && history.redo(map) && history.redo(map));
    CHECK(fingerprint(map) == edited);
}

//removing a layer takes its tiles with it, undo brings them back in the same place
static void testRemoveAndMoveLayer() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(history.addLayer(map, {.name = "Front"}).has_value());
    CHECK(history.place(map, 0, {1, 1}, small).has_value());
    CHECK(history.place(map, 1, {5, 5}, big).has_value());
    history.commit();
    const auto full = fingerprint(map);

    //the last layer can never be removed, a wrong index does nothing
    CHECK(!history.removeLayer(map, 7));

    //remove the first layer: the "Front" layer becomes layer 0
    CHECK(history.removeLayer(map, 0));
    CHECK(map.layerCount() == 1 && map.layerInfo(0).name == "Front");
    CHECK(map.tile(0, {5, 5}) == big);
    CHECK(!history.removeLayer(map, 0));

    CHECK(history.undo(map));
    CHECK(fingerprint(map) == full);
    CHECK(history.redo(map));
    CHECK(map.layerCount() == 1);
    CHECK(history.undo(map));

    //move and undo the move
    CHECK(history.moveLayer(map, 0, 1));
    CHECK(map.layerInfo(0).name == "Front" && map.layerInfo(1).name == "Ground");
    CHECK(!history.moveLayer(map, 1, 1));
    CHECK(history.undo(map));
    CHECK(fingerprint(map) == full);
}

int main() {
    testPlaceUndoRedo();
    testEraseBigTile();
    testGroupedEdits();
    testUndoWithoutCommit();
    testFailedEditsAreNotRecorded();
    testSpawn();
    testCheckpoints();
    testNewEditClearsRedo();
    testDirtyFlag();
    testLimit();
    testClear();
    testLayerEdits();
    testRemoveAndMoveLayer();
    std::println("all history tests passed");
}
