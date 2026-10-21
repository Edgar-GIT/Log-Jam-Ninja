// ButtonMenuView.cpp - implementation of the sprite button menu.

#include "ButtonMenuView.hpp"

#include <cmath>

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

MenuEvent ButtonMenuView::updateMouse(Menu& menu, Vector2 mouse, bool pressed, [[maybe_unused]] bool down) {
    //the mouse only takes control when it really moves (the first frame only remembers where it is)
    const bool moved = m_seenMouse && (mouse.x != m_lastMouse.x || mouse.y != m_lastMouse.y);
    m_lastMouse = mouse;
    m_seenMouse = true;

    if (usesFallback()) {
        return m_fallback.updateMouse(menu, mouse, moved, pressed);
    }

    //a cursor that changed without the mouse was moved by the keyboard, so the keyboard is in control again
    if (menu.cursor() != m_lastCursor) {
        m_mouseMode = false;
    }

    if (moved) {
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
    const Rectangle destination{std::floor(m_stack.centerX - width * 0.5f),
                                std::floor(m_stack.centerY(static_cast<std::size_t>(index)) - height * 0.5f), width,
                                height};
    DrawTexturePro(texture, source, destination, Vector2{0.0f, 0.0f}, 0.0f, item.enabled ? WHITE : kDisabledTint);
}

}  //namespace ljn
