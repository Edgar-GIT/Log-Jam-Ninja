// MapSerializer.cpp - implementation of the json save and load functions.

#include "io/MapSerializer.hpp"

#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>
#include <utility>
#include "io/JsonReflect.hpp"

namespace ljn {
namespace {

using json = nlohmann::json;

//one placed tile as it appears in the file, its field names become the json keys
struct PlacedEntry {
    int tile{};
    int x{};
    int y{};
};

//finds an array by name, or returns nullptr if it is missing or not an array
const json* findArray(const json& root, const char* key) {
    const auto it = root.find(key);
    return (it != root.end() && it->is_array()) ? &*it : nullptr;
}

//reads every field and rebuilds the map, json errors are caught by the caller
std::expected<Map, IoError> buildMap(const json& root) {
    //the file must match the format and cell size this build understands
    if (root.at("version").get<int>() != kFormatVersion) {
        return std::unexpected(IoError::UnsupportedVersion);
    }
    if (root.at("cellSize").get<int>() != kCellSize) {
        return std::unexpected(IoError::CellSizeMismatch);
    }

    //empty map with the saved size
    auto created = Map::create(root.at("width").get<int>(), root.at("height").get<int>());
    if (!created) {
        return std::unexpected(IoError::InvalidMap);
    }
    Map map = std::move(*created);

    //the three lists always exist in files we save
    const json* tiles = findArray(root, "tiles");
    const json* placed = findArray(root, "placed");
    const json* checkpoints = findArray(root, "checkpoints");
    if (tiles == nullptr || placed == nullptr || checkpoints == nullptr) {
        return std::unexpected(IoError::InvalidFormat);
    }

    //palette: the order in the file decides the ids (1, 2, 3...)
    for (const json& item : *tiles) {
        if (!map.addTileDef(fromJsonObject<TileDef>(item))) {
            return std::unexpected(IoError::InvalidTile);
        }
    }

    //placed tiles: place() refuses overlaps and anything outside the map
    for (const json& item : *placed) {
        const auto entry = fromJsonObject<PlacedEntry>(item);
        if (entry.tile < 1 || entry.tile > std::numeric_limits<TileId>::max()) {
            return std::unexpected(IoError::InvalidPlacement);
        }
        if (!map.place({entry.x, entry.y}, static_cast<TileId>(entry.tile))) {
            return std::unexpected(IoError::InvalidPlacement);
        }
    }

    //spawn point is optional
    if (const auto it = root.find("spawn"); it != root.end()) {
        if (!map.setSpawn(fromJsonObject<Cell>(*it))) {
            return std::unexpected(IoError::InvalidObject);
        }
    }

    //checkpoints: duplicates and cells outside the map are refused
    for (const json& item : *checkpoints) {
        if (!map.addCheckpoint(fromJsonObject<Cell>(item))) {
            return std::unexpected(IoError::InvalidObject);
        }
    }

    return map;
}

}  //anonymous namespace

//writes the whole map as pretty json text
std::string toJson(const Map& map) {
    json root;
    root["version"] = kFormatVersion;
    root["cellSize"] = kCellSize;
    root["width"] = map.width();
    root["height"] = map.height();

    //palette, in id order
    json tiles = json::array();
    for (const TileDef& def : map.tileDefs()) {
        tiles.push_back(toJsonObject(def));
    }
    root["tiles"] = std::move(tiles);

    //placed tiles: only the top-left cell of each one, found by offset (0, 0)
    json placed = json::array();
    const auto grid = map.grid();
    for (std::size_t y = 0; y < grid.extent(0); ++y) {
        for (std::size_t x = 0; x < grid.extent(1); ++x) {
            const Map::Slot& slot = grid[y, x];
            if (slot.id != kEmptyTile && slot.dx == 0 && slot.dy == 0) {
                placed.push_back(toJsonObject(PlacedEntry{.tile = slot.id,
                                                          .x = static_cast<int>(x),
                                                          .y = static_cast<int>(y)}));
            }
        }
    }
    root["placed"] = std::move(placed);

    //spawn point, only if one was set
    if (map.hasSpawn()) {
        root["spawn"] = toJsonObject(map.spawn());
    }

    //checkpoints, already sorted left to right
    json checkpoints = json::array();
    for (const Cell c : map.checkpoints()) {
        checkpoints.push_back(toJsonObject(c));
    }
    root["checkpoints"] = std::move(checkpoints);

    //2 spaces of indentation, bad utf-8 is replaced instead of throwing
    std::string text = root.dump(2, ' ', false, json::error_handler_t::replace);
    text += '\n';
    return text;
}

//parses the text, then rebuilds the map from it
std::expected<Map, IoError> fromJson(std::string_view text) {
    const json root = json::parse(text.begin(), text.end(), nullptr, false);
    if (!root.is_object()) {
        return std::unexpected(IoError::ParseFailed);
    }

    //a missing field or a wrong type throws, so we turn that into an error value
    try {
        return buildMap(root);
    } catch (const json::exception&) {
        return std::unexpected(IoError::InvalidFormat);
    }
}

//saves through a temporary file, so a crash never leaves a half written map
std::expected<void, IoError> saveToFile(const Map& map, const std::filesystem::path& path) {
    auto temp = path;
    temp += ".tmp";

    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return std::unexpected(IoError::FileOpenFailed);
        }
        out << toJson(map);
        out.flush();
        if (!out) {
            return std::unexpected(IoError::FileWriteFailed);
        }
    }

    std::error_code ec;
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
        return std::unexpected(IoError::FileWriteFailed);
    }
    return {};
}

//reads the whole file into a string and parses it
std::expected<Map, IoError> loadFromFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::unexpected(IoError::FileOpenFailed);
    }
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return fromJson(text);
}

//maps each error to a short text
std::string_view describe(IoError error) noexcept {
    switch (error) {
        case IoError::FileOpenFailed:     return "Could not open the file";
        case IoError::FileWriteFailed:    return "Could not write the file";
        case IoError::ParseFailed:        return "File is not valid json";
        case IoError::UnsupportedVersion: return "Unsupported map file version";
        case IoError::CellSizeMismatch:   return "Map was made with a different cell size";
        case IoError::InvalidFormat:      return "Map file has missing or wrong fields";
        case IoError::InvalidMap:         return "Map size is invalid";
        case IoError::InvalidTile:        return "A tile in the palette is invalid";
        case IoError::InvalidPlacement:   return "A placed tile is invalid or overlaps another";
        case IoError::InvalidObject:      return "Spawn or checkpoint is invalid";
    }
    return "Unknown error";
}

}  //namespace ljn
