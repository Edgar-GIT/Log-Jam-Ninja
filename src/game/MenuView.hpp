// MenuView.hpp - draws a menu (the logic is in the core) into the picture, and finds what is under the mouse.

#pragma once

#include <raylib.h>
#include "ui/Menu.hpp"

namespace ljn {

class MenuView {
public:
    //a view without an area draws nothing, it is used with an area given on each call
    MenuView() = default;

    //a view with its own area, so the calls do not need to repeat it
    explicit MenuView(const Rectangle& area) noexcept : m_area(area) {}

    //draws the menu inside the area, one row for each item
    void draw(const Menu& menu, const Rectangle& area) const;

    //the same, with a font of its own instead of the font of raylib
    void draw(const Menu& menu, const Rectangle& area, const Font& font) const;

    //draws the menu inside the area of this view
    void draw(const Menu& menu) const;

    //the item under the mouse, -1 if the mouse is not over any of them
    [[nodiscard]] int itemAt(const Menu& menu, Vector2 position, const Rectangle& area) const;

    //how far along the slider the mouse is, from 0 (left end) to 1 (right end)
    [[nodiscard]] float sliderFraction(const Menu& menu, int index, Vector2 position, const Rectangle& area) const;

    //the mouse over the menu: it selects the item under it when the mouse moves, and uses it when it is clicked
    [[nodiscard]] MenuEvent updateMouse(Menu& menu, Vector2 mouse, bool moved, bool pressed, const Rectangle& area);

    //the same, with the area of this view
    [[nodiscard]] MenuEvent updateMouse(Menu& menu, Vector2 mouse, bool moved, bool pressed);

private:
    //draws the menu with a font, null for the font of raylib
    void draw(const Menu& menu, const Rectangle& area, const Font* font) const;

    //the row of an item
    [[nodiscard]] static Rectangle rowRect(const Menu& menu, int index, const Rectangle& area) noexcept;

    //the bar of a slider, on the right side of the row
    [[nodiscard]] static Rectangle sliderRect(const Rectangle& row) noexcept;

    //the area of this view, used when the caller does not give one
    Rectangle m_area{};
};

}  //namespace ljn
