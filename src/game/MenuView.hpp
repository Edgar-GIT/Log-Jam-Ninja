// MenuView.hpp - draws a menu (the logic is in the core) into the picture, and finds what is under the mouse.

#pragma once

#include <raylib.h>
#include "ui/Menu.hpp"

namespace ljn {

class MenuView {
public:
    //draws the menu inside the area, one row for each item
    void draw(const Menu& menu, const Rectangle& area) const;

    //the item under the mouse, -1 if the mouse is not over any of them
    [[nodiscard]] int itemAt(const Menu& menu, Vector2 position, const Rectangle& area) const;

    //how far along the slider the mouse is, from 0 (left end) to 1 (right end)
    [[nodiscard]] float sliderFraction(const Menu& menu, int index, Vector2 position, const Rectangle& area) const;

private:
    //the row of an item
    [[nodiscard]] static Rectangle rowRect(const Menu& menu, int index, const Rectangle& area) noexcept;

    //the bar of a slider, on the right side of the row
    [[nodiscard]] static Rectangle sliderRect(const Rectangle& row) noexcept;
};

}  //namespace ljn
