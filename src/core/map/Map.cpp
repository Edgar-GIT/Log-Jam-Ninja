// Map.cpp - implementation of the Map class declared in Map.hpp.
#include "map/Map.hpp"

// Standard library headers used below.
#include <algorithm>
#include <limits>
#include <utility>

namespace ljn {

// Builds an empty map, or fails if the size is outside the allowed range.
std::expected<Map, MapError> Map::create(int width, int height) {
    if (width < 1 || width > kMaxWidth || height < 1 || height > kMaxHeight) {
        return std::unexpected(MapError::InvalidSize);
    }
    return Map{width, height};
}

// Stores the size and creates width * height empty slots.
Map::Map(int width, int height)
    : m_width{width},
      m_height{height},
      m_slots(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {}

// Returns the tile ID covering the cell, or kEmptyTile if there is none.
TileId Map::tile(Cell c) const noexcept {
    return inBounds(c) ? m_slots[index(c)].id : kEmptyTile;
}

// Walks back from any cell of a tile to the tile's top-left cell.
Cell Map::originOf(Cell c) const noexcept {
    if (!inBounds(c)) {
        return c;
    }
    const Slot& slot = m_slots[index(c)];
    return {c.x - slot.dx, c.y - slot.dy};
}

// A cell is free if it is inside the map and nothing covers it.
bool Map::isFree(Cell c) const noexcept {
    return inBounds(c) && m_slots[index(c)].id == kEmptyTile;
}

// Checks every cell the tile would cover, without changing anything.
bool Map::canPlace(Cell origin, TileId id) const noexcept {
    if (!hasTile(id)) {
        return false;
    }
    const TileDef& def = m_defs[static_cast<std::size_t>(id) - 1];

    for (int dy = 0; dy < def.cellsHigh(); ++dy) {
        for (int dx = 0; dx < def.cellsWide(); ++dx) {
            if (!isFree({origin.x + dx, origin.y + dy})) {
                return false;
            }
        }
    }
    return true;
}

// Places a tile: validates first, then writes every cell it covers.
std::expected<void, PlaceError> Map::place(Cell origin, TileId id) {
    if (!hasTile(id)) {
        return std::unexpected(PlaceError::UnknownTile);
    }
    if (!inBounds(origin)) {
        return std::unexpected(PlaceError::OutOfBounds);
    }

    const TileDef& def = m_defs[static_cast<std::size_t>(id) - 1];
    const int wide = def.cellsWide();
    const int high = def.cellsHigh();

    const Cell last{origin.x + wide - 1, origin.y + high - 1};
    if (!inBounds(last)) {
        return std::unexpected(PlaceError::OutOfBounds);
    }

    for (int dy = 0; dy < high; ++dy) {
        for (int dx = 0; dx < wide; ++dx) {
            if (m_slots[index({origin.x + dx, origin.y + dy})].id != kEmptyTile) {
                return std::unexpected(PlaceError::Overlap);
            }
        }
    }

    for (int dy = 0; dy < high; ++dy) {
        for (int dx = 0; dx < wide; ++dx) {
            m_slots[index({origin.x + dx, origin.y + dy})] =
                Slot{id, static_cast<std::uint8_t>(dx), static_cast<std::uint8_t>(dy)};
        }
    }
    return {};
}

// Erases the whole tile that covers the cell and reports what was removed.
std::optional<PlacedTile> Map::erase(Cell c) {
    if (!inBounds(c)) {
        return std::nullopt;
    }
    const TileId id = m_slots[index(c)].id;
    if (id == kEmptyTile) {
        return std::nullopt;
    }

    const Cell origin = originOf(c);
    const TileDef& def = m_defs[static_cast<std::size_t>(id) - 1];

    for (int dy = 0; dy < def.cellsHigh(); ++dy) {
        for (int dx = 0; dx < def.cellsWide(); ++dx) {
            m_slots[index({origin.x + dx, origin.y + dy})] = Slot{};
        }
    }
    return PlacedTile{origin, id};
}

// Looks up in the solid table whether the tile covering the cell blocks movement.
bool Map::isSolid(Cell c) const noexcept {
    return m_solidLut[tile(c)] != 0;
}

// Adds a tile type to the palette after checking its size.
std::expected<TileId, MapError> Map::addTileDef(TileDef def) {
    if (def.pixelWidth < 1 || def.pixelHeight < 1
        || def.cellsWide() > kMaxTileCells || def.cellsHigh() > kMaxTileCells) {
        return std::unexpected(MapError::InvalidTileSize);
    }
    if (m_defs.size() >= static_cast<std::size_t>(std::numeric_limits<TileId>::max())) {
        return std::unexpected(MapError::PaletteFull);
    }

    m_solidLut.push_back(static_cast<std::uint8_t>(def.solid));
    m_defs.push_back(std::move(def));
    return static_cast<TileId>(m_defs.size());
}

// An ID is valid if it is not 0 and not past the end of the palette.
bool Map::hasTile(TileId id) const noexcept {
    return id != kEmptyTile && static_cast<std::size_t>(id) <= m_defs.size();
}

// Sets the spawn point if the cell is inside the map.
bool Map::setSpawn(Cell c) noexcept {
    if (!inBounds(c)) {
        return false;
    }
    m_spawn = c;
    m_hasSpawn = true;
    return true;
}

// Inserts a checkpoint in sorted position, refusing duplicates and cells outside the map.
bool Map::addCheckpoint(Cell c) {
    if (!inBounds(c)) {
        return false;
    }
    const auto it = std::ranges::lower_bound(m_checkpoints, c);
    if (it != m_checkpoints.end() && *it == c) {
        return false;
    }
    m_checkpoints.insert(it, c);
    return true;
}

// Removes a checkpoint if it exists.
bool Map::removeCheckpoint(Cell c) {
    const auto it = std::ranges::lower_bound(m_checkpoints, c);
    if (it == m_checkpoints.end() || *it != c) {
        return false;
    }
    m_checkpoints.erase(it);
    return true;
}

}  // namespace ljn
