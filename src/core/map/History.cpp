// History.cpp - implementation of the edit history.

#include "map/History.hpp"

#include <ranges>
#include <utility>

namespace ljn {
namespace {

//puts the spawn point where it was (or removes it when there was none)
void setSpawnTo(Map& map, const std::optional<Cell>& spawn) {
    if (spawn) {
        static_cast<void>(map.setSpawn(*spawn));
    } else {
        map.clearSpawn();
    }
}

//doing a change again, and taking it back, one pair of functions per kind of change
void redoChange(const TilePlaced& change, Map& map) {
    static_cast<void>(map.place(change.layer, change.origin, change.id));
}
void undoChange(const TilePlaced& change, Map& map) {
    static_cast<void>(map.erase(change.layer, change.origin));
}

void redoChange(const TileErased& change, Map& map) {
    static_cast<void>(map.erase(change.layer, change.origin));
}
void undoChange(const TileErased& change, Map& map) {
    static_cast<void>(map.place(change.layer, change.origin, change.id));
}

void redoChange(const SpawnChanged& change, Map& map) {
    setSpawnTo(map, change.after);
}
void undoChange(const SpawnChanged& change, Map& map) {
    setSpawnTo(map, change.before);
}

void redoChange(const CheckpointAdded& change, Map& map) {
    static_cast<void>(map.addCheckpoint(change.cell));
}
void undoChange(const CheckpointAdded& change, Map& map) {
    static_cast<void>(map.removeCheckpoint(change.cell));
}

void redoChange(const CheckpointRemoved& change, Map& map) {
    static_cast<void>(map.removeCheckpoint(change.cell));
}
void undoChange(const CheckpointRemoved& change, Map& map) {
    static_cast<void>(map.addCheckpoint(change.cell));
}

void redoChange(const LayerAdded& change, Map& map) {
    static_cast<void>(map.insertLayer(change.index, change.layer));
}
void undoChange(const LayerAdded& change, Map& map) {
    static_cast<void>(map.removeLayer(change.index));
}

void redoChange(const LayerRemoved& change, Map& map) {
    static_cast<void>(map.removeLayer(change.index));
}
void undoChange(const LayerRemoved& change, Map& map) {
    static_cast<void>(map.insertLayer(change.index, change.layer));
}

//moving from a to b is undone by moving from b to a
void redoChange(const LayerMoved& change, Map& map) {
    static_cast<void>(map.moveLayer(change.from, change.to));
}
void undoChange(const LayerMoved& change, Map& map) {
    static_cast<void>(map.moveLayer(change.to, change.from));
}

void redoChange(const LayerEdited& change, Map& map) {
    static_cast<void>(map.setLayerInfo(change.index, change.after));
}
void undoChange(const LayerEdited& change, Map& map) {
    static_cast<void>(map.setLayerInfo(change.index, change.before));
}

}  //anonymous namespace

std::expected<void, PlaceError> History::place(Map& map, int layer, Cell origin, TileId id) {
    const auto result = map.place(layer, origin, id);
    if (result) {
        record(TilePlaced{layer, origin, id});
    }
    return result;
}

std::optional<PlacedTile> History::erase(Map& map, int layer, Cell cell) {
    const auto removed = map.erase(layer, cell);
    if (removed) {
        record(TileErased{layer, removed->origin, removed->id});
    }
    return removed;
}

bool History::setSpawn(Map& map, Cell cell) {
    const std::optional<Cell> before = map.hasSpawn() ? std::optional<Cell>{map.spawn()} : std::nullopt;
    if (before == cell || !map.setSpawn(cell)) {
        return false;
    }
    record(SpawnChanged{before, cell});
    return true;
}

bool History::clearSpawn(Map& map) {
    if (!map.hasSpawn()) {
        return false;
    }
    record(SpawnChanged{map.spawn(), std::nullopt});
    map.clearSpawn();
    return true;
}

bool History::addCheckpoint(Map& map, Cell cell) {
    if (!map.addCheckpoint(cell)) {
        return false;
    }
    record(CheckpointAdded{cell});
    return true;
}

bool History::removeCheckpoint(Map& map, Cell cell) {
    if (!map.removeCheckpoint(cell)) {
        return false;
    }
    record(CheckpointRemoved{cell});
    return true;
}

std::expected<int, MapError> History::addLayer(Map& map, LayerInfo info) {
    commit();
    const auto index = map.addLayer(std::move(info));
    if (!index) {
        return index;
    }

    //the layer is copied so undo can remove it and redo can bring it back
    record(LayerAdded{*index, map.layerAt(*index)});
    commit();
    return index;
}

bool History::removeLayer(Map& map, int index) {
    commit();

    //a map always keeps at least one layer
    if (map.layerCount() <= 1) {
        return false;
    }
    auto removed = map.removeLayer(index);
    if (!removed) {
        return false;
    }
    record(LayerRemoved{index, std::move(*removed)});
    commit();
    return true;
}

bool History::moveLayer(Map& map, int from, int to) {
    commit();
    if (from == to || !map.moveLayer(from, to)) {
        return false;
    }
    record(LayerMoved{from, to});
    commit();
    return true;
}

bool History::editLayer(Map& map, int index, LayerInfo info) {
    commit();
    if (!map.hasLayer(index)) {
        return false;
    }
    LayerInfo before = map.layerInfo(index);
    if (!map.setLayerInfo(index, info)) {
        return false;
    }

    //nothing to remember when nothing changed
    if (before.name == info.name && before.parallaxX == info.parallaxX && before.parallaxY == info.parallaxY
        && before.solid == info.solid && before.visible == info.visible) {
        return false;
    }
    record(LayerEdited{index, std::move(before), std::move(info)});
    commit();
    return true;
}

void History::commit() {
    if (m_pending.empty()) {
        return;
    }

    //a new edit ends the redo branch, and a save point inside that branch can never be reached again
    if (!m_redo.empty()) {
        m_redo.clear();
        if (m_savedAt && *m_savedAt > m_undo.size()) {
            m_savedAt.reset();
        }
    }

    m_undo.push_back(std::move(m_pending));
    m_pending.clear();

    //forget the oldest action when there are too many, the save point moves with the list
    if (m_undo.size() > kMaxActions) {
        m_undo.pop_front();
        if (m_savedAt && *m_savedAt > 0) {
            --*m_savedAt;
        } else {
            m_savedAt.reset();
        }
    }
    ++m_revision;
}

bool History::undo(Map& map) {
    commit();
    if (m_undo.empty()) {
        return false;
    }

    //the changes of the action are taken back from the last to the first
    Action action = std::move(m_undo.back());
    m_undo.pop_back();
    for (const Change& change : action | std::views::reverse) {
        std::visit([&map](const auto& c) { undoChange(c, map); }, change);
    }
    m_redo.push_back(std::move(action));
    ++m_revision;
    return true;
}

bool History::redo(Map& map) {
    commit();
    if (m_redo.empty()) {
        return false;
    }

    //the changes are done again from the first to the last
    Action action = std::move(m_redo.back());
    m_redo.pop_back();
    for (const Change& change : action) {
        std::visit([&map](const auto& c) { redoChange(c, map); }, change);
    }
    m_undo.push_back(std::move(action));
    ++m_revision;
    return true;
}

void History::markSaved() {
    commit();
    m_savedAt = m_undo.size();
}

bool History::isDirty() const noexcept {
    return !m_pending.empty() || m_savedAt != m_undo.size();
}

void History::clear() {
    m_pending.clear();
    m_undo.clear();
    m_redo.clear();
    m_savedAt = 0;
    ++m_revision;
}

}  //namespace ljn
