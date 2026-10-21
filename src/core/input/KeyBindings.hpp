// KeyBindings.hpp - which keys do which action. every action has two keys (main and alternative).

#pragma once

#include <array>
#include <optional>
#include "input/Action.hpp"
#include "input/Keys.hpp"

namespace ljn {

inline constexpr int kSlotsPerAction = 2;

//rules that always hold: a key is used by at most one action, and every action keeps at least one key
class KeyBindings {
public:
    //the default keys: arrows and WASD to move, space and Z and K to jump, X to attack, P to pause
    KeyBindings();

    //every slot of every action, one row for each action in the order of kAllActions
    using Table = std::array<std::array<int, kSlotsPerAction>, kActionCount>;

    //bindings from a table (read from a file), or nothing if the table breaks the rules
    [[nodiscard]] static std::optional<KeyBindings> fromTable(const Table& table) noexcept;

    //the key in a slot (0 main, 1 alternative), key::None if the slot is empty
    [[nodiscard]] int key(Action action, int slot) const noexcept;

    //true if this key is one of the keys of the action
    [[nodiscard]] bool uses(Action action, int key) const noexcept;

    //the action that uses this key, if any
    [[nodiscard]] std::optional<Action> actionUsing(int key) const noexcept;

    //puts a key in a slot. if another action already uses the key, the two slots swap keys
    //returns false and changes nothing if that would leave an action without keys, or the key is not allowed
    bool bind(Action action, int slot, int key) noexcept;

    //empties a slot, returns false and changes nothing if it is the last key of the action
    bool clear(Action action, int slot) noexcept;

    //back to the default keys
    void reset() noexcept { *this = KeyBindings{}; }

    [[nodiscard]] bool operator==(const KeyBindings&) const noexcept = default;

private:
    using Slots = std::array<int, kSlotsPerAction>;

    [[nodiscard]] int keyCount(Action action) const noexcept;

    std::array<Slots, kActionCount> m_keys{};
};

}  //namespace ljn
