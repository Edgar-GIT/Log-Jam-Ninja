// serializer_test.cpp - checks that maps survive save and load, and that bad files are refused.

#include <algorithm>
#include <filesystem>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include "check.hpp"
#include "io/MapSerializer.hpp"
#include "map/Map.hpp"

using namespace ljn;

//builds an empty map for a test
static Map makeMap(int width = 20, int height = 12) {
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

//true if both maps have exactly the same size, palette, cells, spawn and checkpoints
static bool sameMap(const Map& a, const Map& b) {
    if (a.width() != b.width() || a.height() != b.height()) {
        return false;
    }

    //palette
    if (a.tileDefs().size() != b.tileDefs().size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.tileDefs().size(); ++i) {
        const TileDef& da = a.tileDefs()[i];
        const TileDef& db = b.tileDefs()[i];
        if (da.file != db.file || da.pixelWidth != db.pixelWidth
            || da.pixelHeight != db.pixelHeight || da.solid != db.solid) {
            return false;
        }
    }

    //every cell, including the offsets of big tiles
    const auto ga = a.grid();
    const auto gb = b.grid();
    for (std::size_t y = 0; y < ga.extent(0); ++y) {
        for (std::size_t x = 0; x < ga.extent(1); ++x) {
            if (ga[y, x].id != gb[y, x].id || ga[y, x].dx != gb[y, x].dx
                || ga[y, x].dy != gb[y, x].dy) {
                return false;
            }
        }
    }

    //spawn and checkpoints
    if (a.hasSpawn() != b.hasSpawn() || (a.hasSpawn() && a.spawn() != b.spawn())) {
        return false;
    }
    return std::ranges::equal(a.checkpoints(), b.checkpoints());
}

//how many times a piece of text appears inside another
static std::size_t countOf(const std::string& text, std::string_view needle) {
    std::size_t count = 0;
    for (auto pos = text.find(needle); pos != std::string::npos;
         pos = text.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}

//a map with every feature: small, big and non solid tiles, a floor, spawn and checkpoints
static Map makeFullMap() {
    Map map = makeMap();
    const TileId small = addTile(map, 32, 32);
    const TileId big = addTile(map, 128, 128);
    const TileId decor = addTile(map, 64, 32, false);

    for (int x = 0; x < map.width(); ++x) {
        CHECK(map.place({x, 11}, small).has_value());
    }
    CHECK(map.place({4, 2}, big).has_value());
    CHECK(map.place({10, 5}, decor).has_value());
    CHECK(map.setSpawn({1, 10}));
    CHECK(map.addCheckpoint({15, 10}));
    CHECK(map.addCheckpoint({8, 10}));
    return map;
}

//the pieces of a map file, so each test can break exactly one of them
struct Parts {
    int version = kFormatVersion;
    int cellSize = kCellSize;
    int width = 8;
    int height = 6;
    std::string tiles = R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32,"solid":true}])";
    std::string placed = "[]";
    std::string checkpoints = "[]";
    std::string extra;  //anything added at the end, for example a spawn
};

//glues the pieces into json text
static std::string makeJson(const Parts& p) {
    return "{\"version\":" + std::to_string(p.version) + ",\"cellSize\":" + std::to_string(p.cellSize)
         + ",\"width\":" + std::to_string(p.width) + ",\"height\":" + std::to_string(p.height)
         + ",\"tiles\":" + p.tiles + ",\"placed\":" + p.placed
         + ",\"checkpoints\":" + p.checkpoints + p.extra + "}";
}

//save to text and load back gives the same map, and saving again gives the same text
static void testRoundTrip() {
    const Map original = makeFullMap();
    const std::string text = toJson(original);

    const auto loaded = fromJson(text);
    CHECK(loaded.has_value());
    CHECK(sameMap(original, *loaded));
    CHECK(toJson(*loaded) == text);

    //20 floor tiles + 1 big tile + 1 decor tile, the big tile is written once, not 16 times
    CHECK(countOf(text, "\"tile\"") == 22);
}

//a map with no tiles, no spawn and no checkpoints also survives
static void testEmptyMap() {
    const Map original = makeMap(5, 4);
    const std::string text = toJson(original);
    CHECK(countOf(text, "\"spawn\"") == 0);

    const auto loaded = fromJson(text);
    CHECK(loaded.has_value());
    CHECK(sameMap(original, *loaded));
    CHECK(!loaded->hasSpawn());
}

//files: save, load, overwrite, and no temporary file left behind
static void testFiles() {
    const auto path = std::filesystem::temp_directory_path() / "ljn_serializer_test.json";
    auto tempPath = path;
    tempPath += ".tmp";
    std::error_code ec;
    std::filesystem::remove(path, ec);

    //first save
    const Map first = makeFullMap();
    CHECK(saveToFile(first, path).has_value());
    CHECK(std::filesystem::exists(path));
    CHECK(!std::filesystem::exists(tempPath));

    //loading gives the same map
    const auto loadedFirst = loadFromFile(path);
    CHECK(loadedFirst.has_value());
    CHECK(sameMap(first, *loadedFirst));

    //saving a different map over the same file replaces it
    const Map second = makeMap(6, 3);
    CHECK(saveToFile(second, path).has_value());
    const auto loadedSecond = loadFromFile(path);
    CHECK(loadedSecond.has_value());
    CHECK(sameMap(second, *loadedSecond));

    std::filesystem::remove(path, ec);
}

//files that cannot be opened give the right error
static void testFileErrors() {
    const auto missing = std::filesystem::temp_directory_path() / "ljn_no_such_dir" / "map.json";
    CHECK(failsWith(loadFromFile(missing), IoError::FileOpenFailed));
    CHECK(failsWith(saveToFile(makeMap(), missing), IoError::FileOpenFailed));
}

//text that is not a json object is refused
static void testParseErrors() {
    CHECK(failsWith(fromJson(""), IoError::ParseFailed));
    CHECK(failsWith(fromJson("this is not json"), IoError::ParseFailed));
    CHECK(failsWith(fromJson("{\"version\": 1,"), IoError::ParseFailed));
    CHECK(failsWith(fromJson("[1, 2, 3]"), IoError::ParseFailed));
}

//the base file used by the next tests must be valid, otherwise they prove nothing
static void testBaseFileIsValid() {
    const auto loaded = fromJson(makeJson({}));
    CHECK(loaded.has_value());
    CHECK(loaded->width() == 8 && loaded->height() == 6);
}

//wrong version, wrong cell size, bad map size
static void testHeaderErrors() {
    CHECK(failsWith(fromJson(makeJson({.version = 2})), IoError::UnsupportedVersion));
    CHECK(failsWith(fromJson(makeJson({.cellSize = 64})), IoError::CellSizeMismatch));
    CHECK(failsWith(fromJson(makeJson({.width = 0})), IoError::InvalidMap));
    CHECK(failsWith(fromJson(makeJson({.height = -3})), IoError::InvalidMap));
}

//missing fields, wrong types and missing lists
static void testFormatErrors() {
    //the tile has no "solid" field
    CHECK(failsWith(fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32}])"})),
                    IoError::InvalidFormat));

    //the width is text instead of a number
    const std::string wrongType =
        R"({"version":1,"cellSize":32,"width":"8","height":6,"tiles":[],"placed":[],"checkpoints":[]})";
    CHECK(failsWith(fromJson(wrongType), IoError::InvalidFormat));

    //the "placed" list is missing
    const std::string noPlaced =
        R"({"version":1,"cellSize":32,"width":8,"height":6,"tiles":[],"checkpoints":[]})";
    CHECK(failsWith(fromJson(noPlaced), IoError::InvalidFormat));
}

//tile types with a bad size
static void testTileErrors() {
    CHECK(failsWith(fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":0,"pixelHeight":32,"solid":true}])"})),
                    IoError::InvalidTile));
}

//placed tiles that do not exist, overlap or do not fit
static void testPlacementErrors() {
    //id 0, an id that is not in the palette and an id too big for 16 bits
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":0,"x":1,"y":1}])"})), IoError::InvalidPlacement));
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":2,"x":1,"y":1}])"})), IoError::InvalidPlacement));
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":70000,"x":1,"y":1}])"})), IoError::InvalidPlacement));

    //outside the map: past the right edge and a negative position (the map is 8 x 6)
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":1,"x":8,"y":0}])"})), IoError::InvalidPlacement));
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":1,"x":-1,"y":0}])"})), IoError::InvalidPlacement));

    //two tiles in the same cell
    CHECK(failsWith(fromJson(makeJson({.placed = R"([{"tile":1,"x":2,"y":2},{"tile":1,"x":2,"y":2}])"})),
                    IoError::InvalidPlacement));

    //a valid placement still works
    CHECK(fromJson(makeJson({.placed = R"([{"tile":1,"x":7,"y":5}])"})).has_value());
}

//spawn and checkpoints outside the map, and duplicated checkpoints
static void testObjectErrors() {
    CHECK(failsWith(fromJson(makeJson({.extra = R"(,"spawn":{"x":99,"y":0})"})), IoError::InvalidObject));
    CHECK(failsWith(fromJson(makeJson({.checkpoints = R"([{"x":8,"y":0}])"})), IoError::InvalidObject));
    CHECK(failsWith(fromJson(makeJson({.checkpoints = R"([{"x":1,"y":1},{"x":1,"y":1}])"})),
                    IoError::InvalidObject));

    //a valid spawn and valid checkpoints work
    const auto loaded = fromJson(makeJson({.checkpoints = R"([{"x":1,"y":1},{"x":3,"y":1}])",
                                           .extra = R"(,"spawn":{"x":0,"y":0})"}));
    CHECK(loaded.has_value());
    CHECK(loaded->hasSpawn() && loaded->spawn() == Cell{0, 0});
    CHECK(loaded->checkpoints().size() == 2);
}

//every error code has a text for the editor
static void testDescribe() {
    for (int i = 0; i <= static_cast<int>(IoError::InvalidObject); ++i) {
        CHECK(!describe(static_cast<IoError>(i)).empty());
    }
}

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testRoundTrip();
    testEmptyMap();
    testFiles();
    testFileErrors();
    testParseErrors();
    testBaseFileIsValid();
    testHeaderErrors();
    testFormatErrors();
    testTileErrors();
    testPlacementErrors();
    testObjectErrors();
    testDescribe();
    std::println("all serializer tests passed");
}
