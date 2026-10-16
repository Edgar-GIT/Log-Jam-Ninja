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

Map::Map(int width, int height): m_width{width}, m_height{height} {}

const LayerInfo& Map::layerInfo(int layer) const noexcept {
    return m_layers[static_cast<std::size_t>(layer)].info;
}

const Map::Layer& Map::layerAt(int layer) const noexcept {
    return m_layers[static_cast<std::size_t>(layer)];
}

bool Map::isValidLayer(const Layer& layer) const noexcept {
    const std::size_t cells = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    return layer.slots.size() == cells && !layer.info.name.empty() && layer.info.name.size() <= kMaxLayerName;
}

std::expected<int, MapError> Map::addLayer(LayerInfo info) {
    const int index = layerCount();
    const auto cells = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    if (const auto done = insertLayer(index, Layer{std::move(info), std::vector<Slot>(cells)}); !done) {
        return std::unexpected(done.error());
    }
    return index;
}

std::expected<void, MapError> Map::insertLayer(int index, Layer layer) {
    if (layerCount() >= kMaxLayers) {
        return std::unexpected(MapError::TooManyLayers);
    }
    if (index < 0 || index > layerCount() || !isValidLayer(layer)) {
        return std::unexpected(MapError::InvalidLayer);
    }
    m_layers.insert(m_layers.begin() + index, std::move(layer));
    return {};
}

std::optional<Map::Layer> Map::removeLayer(int index) {
    if (!hasLayer(index)) {
        return std::nullopt;
    }
    Layer removed = std::move(m_layers[static_cast<std::size_t>(index)]);
    m_layers.erase(m_layers.begin() + index);
    return removed;
}

bool Map::moveLayer(int from, int to) {
    if (!hasLayer(from) || !hasLayer(to)) {
        return false;
    }

    //rotate shifts everything between the two positions by one
    if (from < to) {
        std::rotate(m_layers.begin() + from, m_layers.begin() + from + 1, m_layers.begin() + to + 1);
    } else if (to < from) {
        std::rotate(m_layers.begin() + to, m_layers.begin() + from, m_layers.begin() + from + 1);
    }
    return true;
}

bool Map::setLayerInfo(int layer, LayerInfo info) {
    if (!hasLayer(layer) || info.name.empty() || info.name.size() > kMaxLayerName) {
        return false;
    }
    m_layers[static_cast<std::size_t>(layer)].info = std::move(info);
    return true;
}

TileId Map::tile(int layer, Cell c) const noexcept {
    return (hasLayer(layer) && inBounds(c)) ? layerAt(layer).slots[index(c)].id : kEmptyTile;
}

Cell Map::originOf(int layer, Cell c) const noexcept {
    if (!hasLayer(layer) || !inBounds(c)) {
        return c;
    }
    const Slot& slot = layerAt(layer).slots[index(c)];
    return {c.x - slot.dx, c.y - slot.dy};
}

bool Map::isFree(int layer, Cell c) const noexcept {
    return hasLayer(layer) && inBounds(c) && layerAt(layer).slots[index(c)].id == kEmptyTile;
}

bool Map::canPlace(int layer, Cell origin, TileId id) const noexcept {
    if (!hasTile(id)) {
        return false;
    }
    const TileDef& tileDef = def(id);

    for (int dy = 0; dy < tileDef.cellsHigh(); ++dy) {
        for (int dx = 0; dx < tileDef.cellsWide(); ++dx) {
            if (!isFree(layer, {origin.x + dx, origin.y + dy})) {
                return false;
            }
        }
    }
    return true;
}

std::expected<void, PlaceError> Map::place(int layer, Cell origin, TileId id) {
    if (!hasLayer(layer)) {
        return std::unexpected(PlaceError::UnknownLayer);
    }
    if (!hasTile(id)) {
        return std::unexpected(PlaceError::UnknownTile);
    }
    if (!inBounds(origin)) {
        return std::unexpected(PlaceError::OutOfBounds);
    }

    const TileDef& tileDef = def(id);
    const int wide = tileDef.cellsWide();
    const int high = tileDef.cellsHigh();

    const Cell last{origin.x + wide - 1, origin.y + high - 1};
    if (!inBounds(last)) {
        return std::unexpected(PlaceError::OutOfBounds);
    }

    std::vector<Slot>& slots = m_layers[static_cast<std::size_t>(layer)].slots;
    for (int dy = 0; dy < high; ++dy) {
        for (int dx = 0; dx < wide; ++dx) {
            if (slots[index({origin.x + dx, origin.y + dy})].id != kEmptyTile) {
                return std::unexpected(PlaceError::Overlap);
            }
        }
    }

    for (int dy = 0; dy < high; ++dy) {
        for (int dx = 0; dx < wide; ++dx) {
            slots[index({origin.x + dx, origin.y + dy})] =
                Slot{id, static_cast<std::uint8_t>(dx), static_cast<std::uint8_t>(dy)};
        }
    }
    return {};
}

std::optional<PlacedTile> Map::erase(int layer, Cell c) {
    if (!hasLayer(layer) || !inBounds(c)) {
        return std::nullopt;
    }
    std::vector<Slot>& slots = m_layers[static_cast<std::size_t>(layer)].slots;
    const TileId id = slots[index(c)].id;
    if (id == kEmptyTile) {
        return std::nullopt;
    }

    const Cell origin = originOf(layer, c);
    const TileDef& tileDef = def(id);

    for (int dy = 0; dy < tileDef.cellsHigh(); ++dy) {
        for (int dx = 0; dx < tileDef.cellsWide(); ++dx) {
            slots[index({origin.x + dx, origin.y + dy})] = Slot{};
        }
    }
    return PlacedTile{origin, id};
}

bool Map::isSolid(Cell c) const noexcept {
    for (int layer = 0; layer < layerCount(); ++layer) {
        if (layerInfo(layer).solid && m_solidLut[tile(layer, c)] != 0) {
            return true;
        }
    }
    return false;
}

bool Map::hasAnyTile(Cell c) const noexcept {
    for (int layer = 0; layer < layerCount(); ++layer) {
        if (tile(layer, c) != kEmptyTile) {
            return true;
        }
    }
    return false;
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
