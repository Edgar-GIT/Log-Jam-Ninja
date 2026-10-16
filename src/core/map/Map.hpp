// Map.hpp - the data model of a level: layers of tiles, palette, spawn and checkpoints.

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <mdspan>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "util/Contracts.hpp"

namespace ljn { 

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

//one tile type in the palette: its image, the part of the image that is the tile, and whether it blocks movement
struct TileDef {
    std::string file;  //path relative to assets/, e.g. "tiles/grass.png"
    int pixelWidth{kCellSize};
    int pixelHeight{kCellSize};
    bool solid{true};

    //where the tile starts inside the image, (0, 0) when the whole image is the tile
    int sourceX{0};
    int sourceY{0};

    //animated tiles: the image of every frame in play order and how long each one stays, empty and 0 for a still tile
    std::vector<std::string> frames{};
    int frameMs{0};

    //true when the tile has at least two frames and a speed
    [[nodiscard]] bool isAnimated() const noexcept { return frames.size() > 1 && frameMs > 0; }

    //how many cells the sprite covers horizontally (rounded up)
    [[nodiscard]] constexpr int cellsWide() const noexcept {
        return (pixelWidth + kCellSize - 1) / kCellSize;
    }

    //how many cells the sprite covers vertically (rounded up)
    [[nodiscard]] constexpr int cellsHigh() const noexcept {
        return (pixelHeight + kCellSize - 1) / kCellSize;
    }
};
//the settings of one layer, it is a plain struct so the serializer can save it with reflection
struct LayerInfo {
    std::string name{"Layer"};

    //how much the layer follows the camera: 1 moves with the world, 0.5 moves half as fast, 0 stays still
    float parallaxX{1.0f};
    float parallaxY{1.0f};

    //only tiles on solid layers can block movement
    bool solid{false};

    //hidden layers are not drawn
    bool visible{true};
};

//a tile that was placed on the map: its top-left cell and its type
struct PlacedTile {
    Cell origin;
    TileId id;
};

//reasons why creating a map or adding a tile type can fail
enum class MapError : std::uint8_t { InvalidSize, InvalidTileSize, PaletteFull, TooManyLayers, InvalidLayer };

//reasons why a tile cannot be placed
enum class PlaceError : std::uint8_t { UnknownTile, OutOfBounds, Overlap, UnknownLayer };

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

    //one layer: its settings and its own grid of cells (row by row)
    struct Layer {
        LayerInfo info;
        std::vector<Slot> slots;
    };

    //largest number of layers and longest layer name
    static constexpr int kMaxLayers = 16;
    static constexpr std::size_t kMaxLayerName = 32;

    //builds a map with no layers yet, or returns an error if the size is invalid
    [[nodiscard]] static std::expected<Map, MapError> create(int width, int height);

    //map size in cells
    [[nodiscard]] constexpr int width() const noexcept { return m_width; }
    [[nodiscard]] constexpr int height() const noexcept { return m_height; }

    //true if the cell lies inside the grid
    [[nodiscard]] constexpr bool inBounds(Cell c) const noexcept {
        return static_cast<std::uint32_t>(c.x) < static_cast<std::uint32_t>(m_width) && static_cast<std::uint32_t>(c.y) < static_cast<std::uint32_t>(m_height);
    }

    //layers are stored from the back (index 0) to the front, which is also the drawing order
    [[nodiscard]] int layerCount() const noexcept { return static_cast<int>(m_layers.size()); }
    [[nodiscard]] bool hasLayer(int layer) const noexcept { return layer >= 0 && layer < layerCount(); }
    [[nodiscard]] const LayerInfo& layerInfo(int layer) const noexcept
        LJN_PRE(hasLayer(layer));
    [[nodiscard]] const Layer& layerAt(int layer) const noexcept
        LJN_PRE(hasLayer(layer));

    //adds a layer in front of all the others and returns its index
    std::expected<int, MapError> addLayer(LayerInfo info);

    //puts a whole layer (with its tiles) at this index, used by undo and by the loader
    std::expected<void, MapError> insertLayer(int index, Layer layer);

    //removes a layer and gives it back with its tiles, nullopt if the index is wrong
    std::optional<Layer> removeLayer(int index);

    //moves a layer to another position, false if an index is wrong
    bool moveLayer(int from, int to);

    //changes the settings of a layer, the name must not be empty or too long
    bool setLayerInfo(int layer, LayerInfo info);

    //tile ID covering this cell in this layer (kEmptyTile if empty, outside the map or no such layer)
    [[nodiscard]] TileId tile(int layer, Cell c) const noexcept;

    //top-left cell of the tile covering this cell (the cell itself if it is empty)
    [[nodiscard]] Cell originOf(int layer, Cell c) const noexcept;

    //true if the cell is inside the map and nothing covers it in this layer
    [[nodiscard]] bool isFree(int layer, Cell c) const noexcept;

    //true if the tile would fit at this origin: inside the map and not overlapping anything in this layer
    [[nodiscard]] bool canPlace(int layer, Cell origin, TileId id) const noexcept;

    //places a tile with its top-left corner at origin, or says why it cannot be placed
    std::expected<void, PlaceError> place(int layer, Cell origin, TileId id);

    //erases the whole tile covering this cell and returns what was removed (if anything)
    std::optional<PlacedTile> erase(int layer, Cell c);

    //true if the cell has a solid tile on any solid layer
    [[nodiscard]] bool isSolid(Cell c) const noexcept;

    //true if any layer has a tile in this cell
    [[nodiscard]] bool hasAnyTile(Cell c) const noexcept;

    //read-only 2D view of one layer, used as grid(layer)[y,x]
    [[nodiscard]] auto grid(int layer) const noexcept
        LJN_PRE(hasLayer(layer))
    {
        return std::mdspan(m_layers[static_cast<std::size_t>(layer)].slots.data(),
                           static_cast<std::size_t>(m_height), static_cast<std::size_t>(m_width));
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

    //converts a cell into its position in the flat slot array of a layer
    [[nodiscard]] constexpr std::size_t index(Cell c) const noexcept
        LJN_PRE(inBounds(c))
    {
        return static_cast<std::size_t>(c.y) * static_cast<std::size_t>(m_width)
             + static_cast<std::size_t>(c.x);
    }

    //true if the layer has the right size and its name is valid
    [[nodiscard]] bool isValidLayer(const Layer& layer) const noexcept;

    //the tile type of an ID that is known to exist
    [[nodiscard]] const TileDef& def(TileId id) const noexcept { return m_defs[static_cast<std::size_t>(id) - 1]; }

    int m_width;
    int m_height;

    //the layers, back to front
    std::vector<Layer> m_layers;

    //the palette, ID n is stored at index n - 1
    std::vector<TileDef> m_defs;

    //fast lookup: m_solidLut[id] is 1 if that tile type is solid, ID 0 is never solid
    std::vector<std::uint8_t> m_solidLut{0};

    //spawn point and the flag saying whether one has been set
    Cell m_spawn{};
    bool m_hasSpawn{false};

    //checkpoints, always sorted
    std::vector<Cell> m_checkpoints;
};

}  //namespace ljn
