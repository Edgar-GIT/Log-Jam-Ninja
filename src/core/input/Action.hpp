// Action.hpp - the things the player can do with the keyboard, in the game (the menus use fixed keys).

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace ljn {

//every action that can have keys of its own
enum class Action { Left, Right, Up, Down, Jump, Attack, Pause };

inline constexpr std::size_t kActionCount = 7;

//all the actions in the order the controls screen shows them
inline constexpr std::array<Action, kActionCount> kAllActions{Action::Left, Action::Right, Action::Up,    Action::Down,
                                                              Action::Jump, Action::Attack, Action::Pause};

//the name shown in the controls screen
[[nodiscard]] constexpr std::string_view actionName(Action action) noexcept {
    switch (action) {
        case Action::Left: return "Move left";
        case Action::Right: return "Move right";
        case Action::Up: return "Up";
        case Action::Down: return "Down";
        case Action::Jump: return "Jump";
        case Action::Attack: return "Attack";
        case Action::Pause: return "Pause";
    }
    return "?";
}

//the short name used in the settings file (it never changes, so old files keep working)
[[nodiscard]] constexpr std::string_view actionId(Action action) noexcept {
    switch (action) {
        case Action::Left: return "left";
        case Action::Right: return "right";
        case Action::Up: return "up";
        case Action::Down: return "down";
        case Action::Jump: return "jump";
        case Action::Attack: return "attack";
        case Action::Pause: return "pause";
    }
    return "";
}

//the action that has this short name, if any
[[nodiscard]] constexpr std::optional<Action> actionFromId(std::string_view id) noexcept {
    for (const Action action : kAllActions) {
        if (actionId(action) == id) {
            return action;
        }
    }
    return std::nullopt;
}

}  //namespace ljn
