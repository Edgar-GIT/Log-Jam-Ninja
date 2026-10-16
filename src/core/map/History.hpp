// History.hpp - every edit of a map goes through here, so it can be undone and redone.

#pragma once

#include <cstddef>
#include <deque>
#include <expected>
#include <optional>
#include <variant>
#include <vector>
#include "map/Map.hpp"

namespace ljn {

//a tile was placed in a layer: undo erases it
struct TilePlaced {
    int layer;
    Cell origin;
    TileId id;
};

//a tile was erased from a layer: undo puts it back
struct TileErased {
    int layer;
    Cell origin;
    TileId id;
};

//the spawn point moved, appeared (before is empty) or was removed (after is empty)
struct SpawnChanged {
    std::optional<Cell> before;
    std::optional<Cell> after;
};

//a checkpoint was added
struct CheckpointAdded {
    Cell cell;
};

//a checkpoint was removed
struct CheckpointRemoved {
    Cell cell;
};

//a layer was added at the front: undo removes it
struct LayerAdded {
    int index;
    Map::Layer layer;
};

//a layer was removed with all its tiles: undo puts it back where it was
struct LayerRemoved {
    int index;
    Map::Layer layer;
};

//a layer moved from one position to another
struct LayerMoved {
    int from;
    int to;
};

//the settings of a layer changed (name, parallax, solid, visible)
struct LayerEdited {
    int index;
    LayerInfo before;
    LayerInfo after;
};

//one small step of an edit, whatever kind it is
using Change = std::variant<TilePlaced, TileErased, SpawnChanged, CheckpointAdded, CheckpointRemoved,
                            LayerAdded, LayerRemoved, LayerMoved, LayerEdited>;

class History {
public:
    //how many actions are remembered, the oldest ones are forgotten after that
    static constexpr std::size_t kMaxActions = 1000;

    //the edits: each one changes the map and remembers how to undo it, edits that fail are not remembered
    std::expected<void, PlaceError> place(Map& map, int layer, Cell origin, TileId id);
    std::optional<PlacedTile> erase(Map& map, int layer, Cell cell);
    bool setSpawn(Map& map, Cell cell);
    bool clearSpawn(Map& map);
    bool addCheckpoint(Map& map, Cell cell);
    bool removeCheckpoint(Map& map, Cell cell);

    //layer edits: each one is its own action (they close the group of edits before and after)
    std::expected<int, MapError> addLayer(Map& map, LayerInfo info);
    bool removeLayer(Map& map, int index);
    bool moveLayer(Map& map, int from, int to);
    bool editLayer(Map& map, int index, LayerInfo info);

    //closes the group of edits made since the last commit, so undo treats them as one action
    void commit();

    //goes back one action, or forward again, false if there is nothing to do
    bool undo(Map& map);
    bool redo(Map& map);

    [[nodiscard]] bool canUndo() const noexcept { return !m_pending.empty() || !m_undo.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !m_redo.empty(); }

    //goes up on every commit, undo and redo, so other code can tell when the map changed
    [[nodiscard]] unsigned revision() const noexcept { return m_revision; }

    //remembers that the map is saved now, isDirty() says if it changed since
    void markSaved();
    [[nodiscard]] bool isDirty() const noexcept;

    //forgets everything, for when a new map is loaded
    void clear();

private:
    using Action = std::vector<Change>;

    void record(Change change) { m_pending.push_back(std::move(change)); }

    //edits made since the last commit
    Action m_pending;

    //finished actions, the newest at the back
    std::deque<Action> m_undo;
    std::vector<Action> m_redo;

    //how many actions were done when the map was saved, empty if that state can no longer be reached
    std::optional<std::size_t> m_savedAt{0};

    unsigned m_revision{0};
};

}  //namespace ljn
