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
    const auto grid = map.grid();
    for (std::size_t y = 0; y < grid.extent(0); ++y) {
        for (std::size_t x = 0; x < grid.extent(1); ++x) {
            out.push_back(grid[y, x].id);
            out.push_back(grid[y, x].dx);
            out.push_back(grid[y, x].dy);
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
    const TileId small = map.addTileDef({.file = "small.png"}).value();
    const TileId big = map.addTileDef({.file = "big.png", .pixelWidth = 128, .pixelHeight = 128}).value();
    return {std::move(map), small, big};
}

//place, undo and redo bring the map back and forth
static void testPlaceUndoRedo() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto empty = fingerprint(map);

    CHECK(history.place(map, {3, 3}, small).has_value());
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
    CHECK(history.place(map, {4, 2}, big).has_value());
    history.commit();
    const auto withTile = fingerprint(map);

    //erase from a cell in the middle of the tile
    const auto removed = history.erase(map, {6, 4});
    CHECK(removed.has_value() && removed->origin == Cell{4, 2});
    history.commit();
    CHECK(map.tile({6, 4}) == kEmptyTile);

    CHECK(history.undo(map));
    CHECK(fingerprint(map) == withTile);
    CHECK(map.tile({7, 5}) == big);
    CHECK(map.originOf({7, 5}) == Cell{4, 2});
}

//edits made before one commit are a single action
static void testGroupedEdits() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    const auto empty = fingerprint(map);

    CHECK(history.place(map, {1, 1}, small).has_value());
    CHECK(history.place(map, {2, 1}, small).has_value());
    CHECK(history.place(map, {3, 1}, small).has_value());
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

    CHECK(history.place(map, {1, 1}, small).has_value());
    CHECK(history.canUndo());
    CHECK(history.undo(map));
    CHECK(fingerprint(map) == empty);
}

//edits that fail change nothing and are not remembered
static void testFailedEditsAreNotRecorded() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(history.place(map, {1, 1}, small).has_value());
    history.commit();

    CHECK(!history.place(map, {1, 1}, small).has_value());
    CHECK(!history.place(map, {99, 1}, small).has_value());
    CHECK(!history.erase(map, {9, 9}).has_value());
    CHECK(!history.setSpawn(map, {99, 99}));
    CHECK(!history.removeCheckpoint(map, {5, 5}));
    CHECK(!history.clearSpawn(map));
    history.commit();

    //only the first placement is in the history
    CHECK(history.undo(map));
    CHECK(map.tile({1, 1}) == kEmptyTile);
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

    CHECK(history.place(map, {1, 1}, small).has_value());
    history.commit();
    CHECK(history.undo(map));
    CHECK(history.canRedo());

    CHECK(history.place(map, {2, 2}, small).has_value());
    history.commit();
    CHECK(!history.canRedo());
}

//the dirty flag follows the saved state
static void testDirtyFlag() {
    [[maybe_unused]] auto [map, small, big] = makeSetup();
    History history;
    CHECK(!history.isDirty());

    //an edit that is not committed yet already counts
    CHECK(history.place(map, {1, 1}, small).has_value());
    CHECK(history.isDirty());
    history.commit();
    CHECK(history.isDirty());

    history.markSaved();
    CHECK(!history.isDirty());

    CHECK(history.place(map, {2, 1}, small).has_value());
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
    CHECK(history.place(map, {3, 3}, small).has_value());
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
    CHECK(history.place(map, {1, 1}, small).has_value());
    history.commit();
    CHECK(history.undo(map));

    history.clear();
    CHECK(!history.canUndo() && !history.canRedo() && !history.isDirty());
}

//runs every test, any failed CHECK ends the program with exit code 1
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
    std::println("all history tests passed");
}
