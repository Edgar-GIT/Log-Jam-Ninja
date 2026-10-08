// Map.hpp - the data model of a level: tile grid, palette, spawn and checkpoints.

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <mdspan>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ljn { //variables space for the map engine

//size of one cell, sprites can be bigger and occupy more cells
inline constexpr int kCellSize = 32;

using TileId = std::uint16_t;

//id 0 means no tile in X or Y
inline constexpr TileId kEmptyTile = 0;

//aposition on the grid in cells, y = 0 is the top row
struct Cell {
    std::int32_t x{};
    std::int32_t y{};

    //all 6 comparations automatized: ==, !=, <, >, <=, >= (compares by order)
    constexpr auto operator<=>(const Cell&) const = default;
};

//one tile type in the palette: its image, its size in pixels and whether it blocks movement
struct TileDef {
    std::string file;  //path relative to assets/, e.g. "tiles/grass.png"
    int pixelWidth{kCellSize};
    int pixelHeight{kCellSize};
    bool solid{true};

    //how many cells the sprite covers horizontally (rounded up)
    [[nodiscard]] constexpr int cellsWide() const noexcept {
        return (pixelWidth + kCellSize - 1) / kCellSize;
    }

    //how many cells the sprite covers vertically (rounded up)
    [[nodiscard]] constexpr int cellsHigh() const noexcept {
        return (pixelHeight + kCellSize - 1) / kCellSize;
    }
};

//a tile that was placed on the map: its top-left cell and its type
struct PlacedTile {
    Cell origin;
    TileId id;
};

//reasons why creating a map or adding a tile type can fail
enum class MapError : std::uint8_t { InvalidSize, InvalidTileSize };

//reasons why a tile cannot be placed
enum class PlaceError : std::uint8_t { UnknownTile, OutOfBounds, Overlap };

//the level itself
class Map {
public:
    //largest allowed map size, in cells
    static constexpr int kMaxWidth  = 4096;
    static constexpr int kMaxHeight = 1024;

    //largest allowed tile size, in cells (limited by the 8-bit offsets in Slot)
    static constexpr int kMaxTileCells = 255;

    //what one grid cell stores: the tile ID and how far the cell is from the tile's top-left cell
    struct Slot {
        TileId id{kEmptyTile};
        std::uint8_t dx{0};
        std::uint8_t dy{0};
    };

    //builds an empty map, or returns an error if the size is invalid
    [[nodiscard]] static std::expected<Map, MapError> create(int width, int height);

    //map size in cells
    [[nodiscard]] constexpr int width() const noexcept { return m_width; }
    [[nodiscard]] constexpr int height() const noexcept { return m_height; }

    //true if the cell lies inside the grid
    [[nodiscard]] constexpr bool inBounds(Cell c) const noexcept {
        return static_cast<std::uint32_t>(c.x) < static_cast<std::uint32_t>(m_width) && static_cast<std::uint32_t>(c.y) < static_cast<std::uint32_t>(m_height);
    }

    //tile ID covering this cell (kEmptyTile if empty or outside the map)
    [[nodiscard]] TileId tile(Cell c) const noexcept;

    //top-left cell of the tile covering this cell (the cell itself if it is empty)
    [[nodiscard]] Cell originOf(Cell c) const noexcept;

    //true if the cell is inside the map and nothing covers it
    [[nodiscard]] bool isFree(Cell c) const noexcept;

    //true if the tile would fit at this origin: inside the map and not overlapping anything
    [[nodiscard]] bool canPlace(Cell origin, TileId id) const noexcept;

    //places a tile with its top-left corner at origin, or says why it cannot be placed
    std::expected<void, PlaceError> place(Cell origin, TileId id);

    //erases the whole tile covering this cell and returns what was removed (if anything)
    std::optional<PlacedTile> erase(Cell c);

    //true if the tile covering this cell blocks movement
    [[nodiscard]] bool isSolid(Cell c) const noexcept;

    //read-only 2D view of the grid, used as grid()[y,x]
    [[nodiscard]] auto grid() const noexcept {
        return std::mdspan(m_slots.data(),static_cast<std::size_t>(m_height), static_cast<std::size_t>(m_width));
    }

    //adds a tile type to the palette and returns its new ID
    std::expected<TileId, MapError> addTileDef(TileDef def);

    //true if the ID exists in the palette
    [[nodiscard]] bool hasTile(TileId id) const noexcept;

    //all palette entries (ID n is stored at index n - 1)
    [[nodiscard]] std::span<const TileDef> tileDefs() const noexcept { return m_defs; }

    //spawn point: where the player starts, and respawns before any checkpoint
    bool setSpawn(Cell c) noexcept;
    void clearSpawn() noexcept { m_hasSpawn = false; }
    [[nodiscard]] bool hasSpawn() const noexcept { return m_hasSpawn; }
    [[nodiscard]] Cell spawn() const noexcept { return m_spawn; }

    //checkpoints: kept sorted from left to right
    bool addCheckpoint(Cell c);
    bool removeCheckpoint(Cell c);
    [[nodiscard]] std::span<const Cell> checkpoints() const noexcept { return m_checkpoints; }

private:
    //only create() can build a Map, so an invalid map can never exist
    Map(int width, int height);

    //converts a cell into its position in the flat slot array
    [[nodiscard]] constexpr std::size_t index(Cell c) const noexcept{
        pre(inBounds(c));
    {
        return static_cast<std::size_t>(c.y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(c.x);
    }

    int m_width;
    int m_height;

    //all cells, row by row, in one contiguous block of memory
    std::vector<Slot> m_slots;

    //the palette, ID n is stored at index n - 1
    std::vector<TileDef> m_defs;

    //fast lookup: m_solidLut[id] is 1 if that tile type is solid,ID 0 is never solid
    std::vector<std::uint8_t> m_solidLut{0};

    //spawn point and the flag saying whether one has been set
    Cell m_spawn{};
    bool m_hasSpawn{false};

    //checkpoints, always sorted
    std::vector<Cell> m_checkpoints;
};

}  //namespace ljn
