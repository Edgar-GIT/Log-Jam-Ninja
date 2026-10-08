// Map.cpp - implementation of the Map class declared in Map.hpp.
#include "map/Map.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace ljn {

std::expected<Map, MapError> Map::create(int width, int height) {
    if (width < 1 || width > kMaxWidth || height < 1 || height > kMaxHeight) {
        return std::unexpected(MapError::InvalidSize);
    }
    return Map{width, height};
}

Map::Map(int width, int height): m_width{width},m_height{height},m_slots(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {}

TileId Map::tile(Cell c) const noexcept {
    return inBounds(c) ? m_slots[index(c)].id : kEmptyTile;
}

Cell Map::originOf(Cell c) const noexcept {
    if (!inBounds(c)) {
        return c;
    }
    const Slot& slot = m_slots[index(c)];
    return {c.x - slot.dx, c.y - slot.dy};
}

bool Map::isFree(Cell c) const noexcept {
    return inBounds(c) && m_slots[index(c)].id == kEmptyTile;
}

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

bool Map::isSolid(Cell c) const noexcept {
    return m_solidLut[tile(c)] != 0;
}

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

bool Map::hasTile(TileId id) const noexcept {
    return id != kEmptyTile && static_cast<std::size_t>(id) <= m_defs.size();
}

bool Map::setSpawn(Cell c) noexcept {
    if (!inBounds(c)) {
        return false;
    }
    m_spawn = c;
    m_hasSpawn = true;
    return true;
}

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

bool Map::removeCheckpoint(Cell c) {
    const auto it = std::ranges::lower_bound(m_checkpoints, c);
    if (it == m_checkpoints.end() || *it != c) {
        return false;
    }
    m_checkpoints.erase(it);
    return true;
}

}  // namespace ljn
