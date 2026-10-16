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
    Map map = Map::create(width, height).value();
    CHECK(map.addLayer({.name = "Ground", .solid = true}).has_value());
    return map;
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

    //palette, including the cut out position and the animation of every tile
    if (a.tileDefs().size() != b.tileDefs().size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.tileDefs().size(); ++i) {
        const TileDef& da = a.tileDefs()[i];
        const TileDef& db = b.tileDefs()[i];
        if (da.file != db.file || da.pixelWidth != db.pixelWidth
            || da.pixelHeight != db.pixelHeight || da.solid != db.solid
            || da.sourceX != db.sourceX || da.sourceY != db.sourceY
            || da.frames != db.frames || da.frameMs != db.frameMs) {
            return false;
        }
    }

    //every layer: settings and every cell, including the offsets of big tiles
    if (a.layerCount() != b.layerCount()) {
        return false;
    }
    for (int layer = 0; layer < a.layerCount(); ++layer) {
        const LayerInfo& ia = a.layerInfo(layer);
        const LayerInfo& ib = b.layerInfo(layer);
        if (ia.name != ib.name || ia.parallaxX != ib.parallaxX || ia.parallaxY != ib.parallaxY
            || ia.solid != ib.solid || ia.visible != ib.visible) {
            return false;
        }

        const auto ga = a.grid(layer);
        const auto gb = b.grid(layer);
        for (std::size_t y = 0; y < ga.extent(0); ++y) {
            for (std::size_t x = 0; x < ga.extent(1); ++x) {
                if (ga[y, x].id != gb[y, x].id || ga[y, x].dx != gb[y, x].dx
                    || ga[y, x].dy != gb[y, x].dy) {
                    return false;
                }
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

//a map with every feature: small, big, cut out and animated tiles, a floor, spawn and checkpoints
static Map makeFullMap() {
    Map map = makeMap();
    const TileId small = addTile(map, 32, 32);
    const TileId big = addTile(map, 128, 128);

    //a decor tile cut out of a sheet, it starts at (128, 64) inside the image
    const TileId decor = map.addTileDef({.file = "tiles/sheet.png",
                                         .pixelWidth = 64,
                                         .pixelHeight = 32,
                                         .solid = false,
                                         .sourceX = 128,
                                         .sourceY = 64}).value();

    //an animated tile: three frames, 120 ms each
    const TileId torch = map.addTileDef({.file = "tiles/torch/0.png",
                                         .solid = false,
                                         .frames = {"tiles/torch/0.png", "tiles/torch/1.png", "tiles/torch/2.png"},
                                         .frameMs = 120}).value();

    for (int x = 0; x < map.width(); ++x) {
        CHECK(map.place(0, {x, 11}, small).has_value());
    }
    CHECK(map.place(0, {4, 2}, big).has_value());
    CHECK(map.place(0, {10, 5}, decor).has_value());
    CHECK(map.place(0, {13, 5}, torch).has_value());

    //a background layer with parallax, a hidden front layer, and a tile in each
    const auto back = map.addLayer({.name = "Mountains", .parallaxX = 0.25f, .parallaxY = 0.5f});
    const auto front = map.addLayer({.name = "Fog", .parallaxX = 1.5f, .visible = false});
    CHECK(back.has_value() && front.has_value());
    CHECK(map.place(*back, {6, 6}, decor).has_value());
    CHECK(map.place(*front, {7, 7}, small).has_value());

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
    std::string tiles =
        R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32,"solid":true,"sourceX":0,"sourceY":0,"frames":[],"frameMs":0}])";
    std::string placed = "[]";    //the placed tiles of the only layer
    std::string layers{};         //the whole layer list, when empty one solid layer with "placed" is used
    std::string checkpoints = "[]";
    std::string extra{};  //anything added at the end, for example a spawn
};

//glues the pieces into json text
static std::string makeJson(const Parts& p) {
    const std::string layers = p.layers.empty()
        ? R"([{"name":"Ground","parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true,"placed":)" + p.placed + "}]"
        : p.layers;
    return "{\"version\":" + std::to_string(p.version) + ",\"cellSize\":" + std::to_string(p.cellSize)
         + ",\"width\":" + std::to_string(p.width) + ",\"height\":" + std::to_string(p.height)
         + ",\"tiles\":" + p.tiles + ",\"layers\":" + layers
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

    //20 floor tiles + 1 big tile + 1 decor tile + 1 torch + 2 tiles in the other layers, the big tile is written once, not 16 times
    CHECK(countOf(text, "\"tile\"") == 25);
    CHECK(countOf(text, "\"parallaxX\"") == 3);

    //the settings of the extra layers survive
    CHECK(loaded->layerCount() == 3);
    CHECK(loaded->layerInfo(1).name == "Mountains" && loaded->layerInfo(1).parallaxX == 0.25f);
    CHECK(loaded->layerInfo(2).name == "Fog" && !loaded->layerInfo(2).visible && loaded->layerInfo(2).parallaxX == 1.5f);

    //the cut out tile keeps the place it starts in the image
    CHECK(loaded->tileDefs()[2].sourceX == 128 && loaded->tileDefs()[2].sourceY == 64);

    //the animated tile keeps its frames and speed
    const TileDef& torch = loaded->tileDefs()[3];
    CHECK(torch.isAnimated() && torch.frames.size() == 3 && torch.frameMs == 120);
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
    CHECK(failsWith(fromJson(makeJson({.version = 1})), IoError::UnsupportedVersion));
    CHECK(failsWith(fromJson(makeJson({.cellSize = 64})), IoError::CellSizeMismatch));
    CHECK(failsWith(fromJson(makeJson({.width = 0})), IoError::InvalidMap));
    CHECK(failsWith(fromJson(makeJson({.height = -3})), IoError::InvalidMap));
}

//missing fields, wrong types and missing lists
static void testFormatErrors() {
    //the tile has no "solid" field
    CHECK(failsWith(
        fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32,"sourceX":0,"sourceY":0,"frames":[],"frameMs":0}])"})),
        IoError::InvalidFormat));

    //"frames" is text instead of a list, and "frameMs" is missing
    CHECK(failsWith(
        fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32,"solid":true,"sourceX":0,"sourceY":0,"frames":"a.png","frameMs":0}])"})),
        IoError::InvalidFormat));
    CHECK(failsWith(
        fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":32,"pixelHeight":32,"solid":true,"sourceX":0,"sourceY":0,"frames":[]}])"})),
        IoError::InvalidFormat));

    //the width is text instead of a number
    const std::string wrongType =
        R"({"version":2,"cellSize":32,"width":"8","height":6,"tiles":[],"layers":[],"checkpoints":[]})";
    CHECK(failsWith(fromJson(wrongType), IoError::InvalidFormat));

    //the "layers" list is missing
    const std::string noLayers =
        R"({"version":2,"cellSize":32,"width":8,"height":6,"tiles":[],"checkpoints":[]})";
    CHECK(failsWith(fromJson(noLayers), IoError::InvalidFormat));

    //a layer without its "placed" list, and a layer without a "name"
    CHECK(failsWith(fromJson(makeJson({.layers = R"([{"name":"A","parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true}])"})),
                    IoError::InvalidFormat));
    CHECK(failsWith(fromJson(makeJson({.layers = R"([{"parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true,"placed":[]}])"})),
                    IoError::InvalidFormat));
}

//tile types with a bad size
static void testTileErrors() {
    CHECK(failsWith(
        fromJson(makeJson({.tiles = R"([{"file":"a.png","pixelWidth":0,"pixelHeight":32,"solid":true,"sourceX":0,"sourceY":0,"frames":[],"frameMs":0}])"})),
        IoError::InvalidTile));
}

//a tile with frames loads as an animated tile
static void testAnimatedTile() {
    const auto loaded = fromJson(makeJson(
        {.tiles = R"([{"file":"a/0.png","pixelWidth":32,"pixelHeight":32,"solid":false,"sourceX":0,"sourceY":0,"frames":["a/0.png","a/1.png"],"frameMs":80}])"}));
    CHECK(loaded.has_value());
    CHECK(loaded->tileDefs()[0].isAnimated());
    CHECK(loaded->tileDefs()[0].frames.size() == 2 && loaded->tileDefs()[0].frameMs == 80);
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

//layers with bad settings, too many layers and no layers at all
static void testLayerErrors() {
    CHECK(failsWith(fromJson(makeJson({.layers = "[]"})), IoError::InvalidLayer));
    CHECK(failsWith(fromJson(makeJson({.layers = R"([{"name":"","parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true,"placed":[]}])"})),
                    IoError::InvalidLayer));

    //a name that is too long
    const std::string longName(Map::kMaxLayerName + 1, 'a');
    CHECK(failsWith(fromJson(makeJson({.layers = R"([{"name":")" + longName + R"(","parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true,"placed":[]}])"})),
                    IoError::InvalidLayer));

    //more layers than the limit
    std::string many = "[";
    for (int i = 0; i <= Map::kMaxLayers; ++i) {
        many += std::string(i == 0 ? "" : ",")
              + R"({"name":"L","parallaxX":1.0,"parallaxY":1.0,"solid":false,"visible":true,"placed":[]})";
    }
    many += "]";
    CHECK(failsWith(fromJson(makeJson({.layers = many})), IoError::InvalidLayer));

    //two layers is fine, and the same tile can be in the same cell of both
    const std::string two =
        R"([{"name":"A","parallaxX":0.5,"parallaxY":1.0,"solid":false,"visible":true,"placed":[{"tile":1,"x":1,"y":1}]},)"
        R"({"name":"B","parallaxX":1.0,"parallaxY":1.0,"solid":true,"visible":true,"placed":[{"tile":1,"x":1,"y":1}]}])";
    const auto loaded = fromJson(makeJson({.layers = two}));
    CHECK(loaded.has_value() && loaded->layerCount() == 2);
    CHECK(loaded->tile(0, {1, 1}) == 1 && loaded->tile(1, {1, 1}) == 1);
    CHECK(loaded->isSolid({1, 1}));

    //a tile that overlaps inside one layer is still refused
    const std::string overlap =
        R"([{"name":"A","parallaxX":1.0,"parallaxY":1.0,"solid":false,"visible":true,"placed":[{"tile":1,"x":1,"y":1},{"tile":1,"x":1,"y":1}]}])";
    CHECK(failsWith(fromJson(makeJson({.layers = overlap})), IoError::InvalidPlacement));
}

//every error code has a text for the editor
static void testDescribe() {
    for (int i = 0; i <= static_cast<int>(IoError::InvalidLayer); ++i) {
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
    testAnimatedTile();
    testPlacementErrors();
    testObjectErrors();
    testLayerErrors();
    testDescribe();
    std::println("all serializer tests passed");
}
