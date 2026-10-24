// ButtonMenuView.cpp - implementation of the sprite button menu.

#include "ButtonMenuView.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ljn {
namespace {

//a button that cannot be used is drawn darker
constexpr Color kDisabledTint{110, 110, 120, 255};

}  //anonymous namespace

int ButtonMenuView::rowOf(int itemId) const noexcept {
    for (const Row& entry : m_rows) {
        if (entry.id == itemId) {
            return entry.row;
        }
    }
    return -1;
}

bool ButtonMenuView::isHot(const Menu& menu, int index) const noexcept {
    if (index != menu.cursor()) {
        return false;
    }
    //with the mouse in control only the button under the mouse is hot, when the mouse leaves everything goes back to idle
    return !m_mouseMode || index == m_mouseIndex;
}

MenuEvent ButtonMenuView::updateMouse(Menu& menu, Vector2 mouse, bool moved, bool pressed,
                                      [[maybe_unused]] bool down) {
    if (usesFallback()) {
        return m_fallback.updateMouse(menu, mouse, moved, pressed);
    }

    //a cursor that changed without the mouse was moved by the keyboard, so the keyboard is in control again
    if (menu.cursor() != m_lastCursor) {
        m_mouseMode = false;
    }

    //the mouse only takes control after the hold (a click ends it at once), so the title opens on play
    if (moved && (pressed || GetTime() >= m_mouseHoldUntil)) {
        m_mouseMode = true;
    }

    m_mouseIndex = m_mouseMode ? m_stack.indexAt(mouse.x, mouse.y, menu.size()) : -1;

    MenuEvent event;
    if (m_mouseIndex >= 0) {
        if (moved) {
            menu.hover(m_mouseIndex);
        }
        if (pressed) {
            event = menu.click(m_mouseIndex);
        }
    }
    m_lastCursor = menu.cursor();
    return event;
}

MenuEvent ButtonMenuView::updateKeys(Menu& menu, const MenuInput& input) {
    //without the sheet the plain menu is a column, the list order is the screen order
    if (usesFallback()) {
        return menu.update(input);
    }

    //the arrows are used here, the menu only sees confirm and back
    MenuInput keys = input;
    keys.up = false;
    keys.down = false;
    keys.left = false;
    keys.right = false;

    const auto move = [this, &menu](Direction direction) {
        const int next = neighbour(m_stack, static_cast<std::size_t>(std::max(0, menu.cursor())), menu.size(),
                                   direction);
        if (next >= 0) {
            menu.setCursor(next);
        }
    };
    if (input.up) {
        move(Direction::Up);
    }
    if (input.down) {
        move(Direction::Down);
    }
    if (input.left) {
        move(Direction::Left);
    }
    if (input.right) {
        move(Direction::Right);
    }
    return menu.update(keys);
}

void ButtonMenuView::draw(const Menu& menu, double seconds) const {
    if (usesFallback()) {
        m_fallback.draw(menu);
        return;
    }

    //the buttons that are not hot first, the hot one last, so its glow is on top of its neighbours
    for (const bool hotPass : {false, true}) {
        for (int i = 0; i < static_cast<int>(menu.size()); ++i) {
            if (isHot(menu, i) == hotPass) {
                drawButton(menu, i, seconds);
            }
        }
    }
}

void ButtonMenuView::drawButton(const Menu& menu, int index, double seconds) const {
    const MenuItem& item = menu.items()[static_cast<std::size_t>(index)];
    const int row = rowOf(item.id);
    if (row < 0 || row >= m_layout.rows) {
        return;
    }

    //the frame of the sheet: hover frames when hot, idle frames otherwise
    const Texture2D& texture = m_sheet->texture;
    const ButtonLook look = isHot(menu, index) ? ButtonLook::Hover : ButtonLook::Idle;
    const int column = frameColumn(m_layout, m_animation, look, seconds);
    const PixelRect cell = frameSource(texture.width, texture.height, m_layout, row, column);

    //centered on its place in the stack, on whole pixels so the picture stays sharp
    const float width = cell.width * m_scale;
    const float height = cell.height * m_scale;
    const Rectangle source{cell.x, cell.y, cell.width, cell.height};
    const Rectangle destination{std::floor(m_stack.centerXOf(static_cast<std::size_t>(index), menu.size()) - width * 0.5f),
                                std::floor(m_stack.centerY(static_cast<std::size_t>(index), menu.size()) - height * 0.5f),
                                width, height};
    DrawTexturePro(texture, source, destination, Vector2{0.0f, 0.0f}, 0.0f, item.enabled ? WHITE : kDisabledTint);
}

}  //namespace ljn
