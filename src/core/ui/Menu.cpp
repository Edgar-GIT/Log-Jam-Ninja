// Menu.cpp - implementation of the menu logic.

#include "ui/Menu.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ljn {
namespace {

//true if the cursor can stand on this item
bool selectable(const MenuItem& item) noexcept {
    return item.kind != ItemKind::Label;
}

}  //anonymous namespace

MenuItem Menu::label(std::string text) {
    return {.kind = ItemKind::Label, .id = -1, .label = std::move(text)};
}

MenuItem Menu::button(int id, std::string text) {
    return {.kind = ItemKind::Button, .id = id, .label = std::move(text)};
}

MenuItem Menu::toggle(int id, std::string text, bool on) {
    return {.kind = ItemKind::Toggle, .id = id, .label = std::move(text), .value = on ? 1 : 0};
}

MenuItem Menu::slider(int id, std::string text, int value, int min, int max, int step) {
    return {.kind = ItemKind::Slider,
            .id = id,
            .label = std::move(text),
            .value = std::clamp(value, min, max),
            .min = min,
            .max = max,
            .step = std::max(step, 1)};
}

MenuItem Menu::choice(int id, std::string text, std::vector<std::string> choices, int selected) {
    const int last = std::max(static_cast<int>(choices.size()) - 1, 0);
    return {.kind = ItemKind::Choice,
            .id = id,
            .label = std::move(text),
            .value = std::clamp(selected, 0, last),
            .min = 0,
            .max = last,
            .choices = std::move(choices)};
}

void Menu::add(MenuItem item) {
    m_items.push_back(std::move(item));

    //the cursor starts on the first item it can stand on
    if (!selectable(m_items[static_cast<std::size_t>(m_cursor)])) {
        move(1);
    }
}

void Menu::clear() noexcept {
    m_items.clear();
    m_cursor = 0;
}

const MenuItem* Menu::selected() const noexcept {
    return m_items.empty() ? nullptr : &m_items[static_cast<std::size_t>(m_cursor)];
}

void Menu::setCursor(int index) noexcept {
    if (index >= 0 && index < static_cast<int>(m_items.size()) && selectable(m_items[static_cast<std::size_t>(index)])) {
        m_cursor = index;
    }
}

void Menu::move(int direction) noexcept {
    const int count = static_cast<int>(m_items.size());
    if (count == 0) {
        return;
    }

    //at most one full turn, so a menu with only labels does not loop forever
    for (int tries = 0; tries < count; ++tries) {
        m_cursor = (m_cursor + direction + count) % count;
        if (selectable(m_items[static_cast<std::size_t>(m_cursor)])) {
            return;
        }
    }
}

MenuEvent Menu::use() {
    if (m_items.empty()) {
        return {};
    }
    MenuItem& item = m_items[static_cast<std::size_t>(m_cursor)];
    if (!item.enabled) {
        return {};
    }

    switch (item.kind) {
        case ItemKind::Button:
            return {EventKind::Activated, item.id};
        case ItemKind::Toggle:
            item.value = 1 - item.value;
            return {EventKind::Changed, item.id};
        case ItemKind::Choice:
            //confirm goes to the next choice and wraps around
            if (item.choices.size() < 2) {
                return {};
            }
            item.value = (item.value + 1) % static_cast<int>(item.choices.size());
            return {EventKind::Changed, item.id};
        case ItemKind::Slider:
        case ItemKind::Label:
            return {};
    }
    return {};
}

MenuEvent Menu::change(int direction) {
    if (m_items.empty()) {
        return {};
    }
    MenuItem& item = m_items[static_cast<std::size_t>(m_cursor)];
    if (!item.enabled) {
        return {};
    }

    switch (item.kind) {
        case ItemKind::Slider: {
            const int value = std::clamp(item.value + direction * item.step, item.min, item.max);
            if (value == item.value) {
                return {};
            }
            item.value = value;
            return {EventKind::Changed, item.id};
        }
        case ItemKind::Toggle: {
            //left means off and right means on
            const int value = direction > 0 ? 1 : 0;
            if (value == item.value) {
                return {};
            }
            item.value = value;
            return {EventKind::Changed, item.id};
        }
        case ItemKind::Choice: {
            if (item.choices.size() < 2) {
                return {};
            }
            const int count = static_cast<int>(item.choices.size());
            item.value = (item.value + direction + count) % count;
            return {EventKind::Changed, item.id};
        }
        case ItemKind::Button:
        case ItemKind::Label:
            return {};
    }
    return {};
}

MenuEvent Menu::update(const MenuInput& input) {
    if (input.back) {
        return {EventKind::Back, -1};
    }
    if (input.up) {
        move(-1);
    }
    if (input.down) {
        move(1);
    }
    if (input.left) {
        if (const MenuEvent event = change(-1); event.kind != EventKind::None) {
            return event;
        }
    }
    if (input.right) {
        if (const MenuEvent event = change(1); event.kind != EventKind::None) {
            return event;
        }
    }
    if (input.confirm) {
        return use();
    }
    return {};
}

MenuEvent Menu::click(int index, float fraction) {
    if (index < 0 || index >= static_cast<int>(m_items.size()) || !selectable(m_items[static_cast<std::size_t>(index)])) {
        return {};
    }
    m_cursor = index;

    MenuItem& item = m_items[static_cast<std::size_t>(index)];
    if (item.kind != ItemKind::Slider) {
        return use();
    }
    if (!item.enabled) {
        return {};
    }

    //the click position gives the value, rounded to the step
    const float position = std::clamp(fraction, 0.0f, 1.0f);
    const float raw = static_cast<float>(item.min) + position * static_cast<float>(item.max - item.min);
    const int steps = static_cast<int>(std::lround((raw - static_cast<float>(item.min)) / static_cast<float>(item.step)));
    const int value = std::clamp(item.min + steps * item.step, item.min, item.max);
    if (value == item.value) {
        return {};
    }
    item.value = value;
    return {EventKind::Changed, item.id};
}

MenuItem* Menu::find(int id) noexcept {
    const auto found = std::ranges::find(m_items, id, &MenuItem::id);
    return found != m_items.end() ? &*found : nullptr;
}

const MenuItem* Menu::find(int id) const noexcept {
    const auto found = std::ranges::find(m_items, id, &MenuItem::id);
    return found != m_items.end() ? &*found : nullptr;
}

}  //namespace ljn
