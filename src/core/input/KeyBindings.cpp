// KeyBindings.cpp - implementation of the key bindings.

#include "input/KeyBindings.hpp"

namespace ljn {
namespace {

constexpr std::size_t index(Action action) noexcept {
    return static_cast<std::size_t>(action);
}

bool validSlot(int slot) noexcept {
    return slot >= 0 && slot < kSlotsPerAction;
}

}  //anonymous namespace

KeyBindings::KeyBindings() {
    m_keys[index(Action::Left)] = {key::A, key::Left};
    m_keys[index(Action::Right)] = {key::D, key::Right};
    m_keys[index(Action::Up)] = {key::W, key::Up};
    m_keys[index(Action::Down)] = {key::S, key::Down};
    m_keys[index(Action::Jump)] = {key::Space, key::Z};
    m_keys[index(Action::Attack)] = {key::X, key::K};
    m_keys[index(Action::Pause)] = {key::P, key::None};
}

std::optional<KeyBindings> KeyBindings::fromTable(const Table& table) noexcept {
    //start empty, the default keys would be in the way
    KeyBindings result;
    result.m_keys = {};

    for (std::size_t a = 0; a < kActionCount; ++a) {
        for (std::size_t s = 0; s < kSlotsPerAction; ++s) {
            const int code = table[a][s];
            if (code == key::None) {
                continue;
            }

            //a key that is not allowed, or that is already used, breaks the rules
            if (isReservedKey(code) || result.actionUsing(code).has_value()) {
                return std::nullopt;
            }
            result.m_keys[a][s] = code;
        }
    }

    //every action needs at least one key
    for (const Action action : kAllActions) {
        if (result.keyCount(action) == 0) {
            return std::nullopt;
        }
    }
    return result;
}

int KeyBindings::key(Action action, int slot) const noexcept {
    return validSlot(slot) ? m_keys[index(action)][static_cast<std::size_t>(slot)] : key::None;
}

bool KeyBindings::uses(Action action, int key) const noexcept {
    if (key == key::None) {
        return false;
    }
    for (const int bound : m_keys[index(action)]) {
        if (bound == key) {
            return true;
        }
    }
    return false;
}

std::optional<Action> KeyBindings::actionUsing(int key) const noexcept {
    for (const Action action : kAllActions) {
        if (uses(action, key)) {
            return action;
        }
    }
    return std::nullopt;
}

int KeyBindings::keyCount(Action action) const noexcept {
    int count = 0;
    for (const int bound : m_keys[index(action)]) {
        count += bound != key::None ? 1 : 0;
    }
    return count;
}

bool KeyBindings::bind(Action action, int slot, int newKey) noexcept {
    if (!validSlot(slot) || newKey == key::None || isReservedKey(newKey)) {
        return false;
    }
    auto& slots = m_keys[index(action)];
    const int oldKey = slots[static_cast<std::size_t>(slot)];
    if (oldKey == newKey) {
        return true;
    }

    //the key is already used by another slot
    for (const Action other : kAllActions) {
        auto& otherSlots = m_keys[index(other)];
        for (std::size_t i = 0; i < otherSlots.size(); ++i) {
            if (otherSlots[i] != newKey) {
                continue;
            }

            //the same action, other slot: the two slots swap
            //another action: it gets the key this slot had, and it must not end up empty
            if (other != action && oldKey == key::None && keyCount(other) == 1) {
                return false;
            }
            otherSlots[i] = oldKey;
            slots[static_cast<std::size_t>(slot)] = newKey;
            return true;
        }
    }

    slots[static_cast<std::size_t>(slot)] = newKey;
    return true;
}

bool KeyBindings::clear(Action action, int slot) noexcept {
    if (!validSlot(slot)) {
        return false;
    }
    auto& slots = m_keys[index(action)];
    if (slots[static_cast<std::size_t>(slot)] == key::None) {
        return true;
    }
    if (keyCount(action) <= 1) {
        return false;
    }
    slots[static_cast<std::size_t>(slot)] = key::None;
    return true;
}

}  //namespace ljn
