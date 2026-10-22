// ButtonMenuView.hpp - shows a Menu as sprite buttons from a sprite sheet (the title screen), and turns the mouse into menu events.
// the sheet has one row for each button and the columns are the frames: one idle and one hover (see ui/ButtonSheet.hpp).
// if the sheet image does not exist yet, the plain placeholder menu is shown instead, so the game always works.

#pragma once

#include <vector>
#include <raylib.h>
#include "Assets.hpp"
#include "MenuView.hpp"
#include "ui/ButtonSheet.hpp"
#include "ui/Menu.hpp"

namespace ljn {

class ButtonMenuView {
public:
    //which row of the sheet an item uses, the item is found by its id
    struct Row {
        int id;
        int row;
    };

    ButtonMenuView() = default;

    //stack: where the buttons are on the 960x540 picture. rows: the sheet row of each item.
    //fallbackArea: where the placeholder menu goes when there is no sheet
    ButtonMenuView(ButtonStack stack, std::vector<Row> rows, Rectangle fallbackArea)
        : m_stack(stack), m_rows(std::move(rows)), m_fallback(fallbackArea) {}

    //the image with all the buttons, it must stay alive as long as this view is used
    void setSheet(const Sprite* sheet) noexcept { m_sheet = sheet; }

    //the picture is drawn at this size compared with the sheet (1 means the sheet pixels are screen pixels)
    void setScale(float scale) noexcept { m_scale = scale; }

    //the mouse in picture pixels: moving over a button selects it, a click uses it
    MenuEvent updateMouse(Menu& menu, Vector2 mouse, bool pressed, bool down);

    //the keys: the arrows move by where the buttons are on the screen (the sheet is not a plain column),
    //confirm and back are passed to the menu
    MenuEvent updateKeys(Menu& menu, const MenuInput& input);

    //seconds is any clock that keeps running (the animation frames come from it)
    void draw(const Menu& menu, double seconds) const;

private:
    //no sheet (or a placeholder in its place) means the plain menu is used
    [[nodiscard]] bool usesFallback() const noexcept { return m_sheet == nullptr || m_sheet->missing; }

    //the row of the sheet for an item, -1 if the item has none
    [[nodiscard]] int rowOf(int itemId) const noexcept;

    //the button looks "hot" (hover look) when the cursor is on it, but with the mouse only while the mouse is really over it
    [[nodiscard]] bool isHot(const Menu& menu, int index) const noexcept;

    void drawButton(const Menu& menu, int index, double seconds) const;

    ButtonStack m_stack;
    SheetLayout m_layout;
    SheetAnimation m_animation;
    std::vector<Row> m_rows;
    MenuView m_fallback{Rectangle{0.0f, 0.0f, 0.0f, 0.0f}};
    const Sprite* m_sheet{nullptr};
    float m_scale{1.0f};

    //the mouse: the last position, the button under it, and whether the mouse (and not the keyboard) is in control
    Vector2 m_lastMouse{0.0f, 0.0f};
    bool m_seenMouse{false};
    bool m_mouseMode{false};
    int m_mouseIndex{-1};
    int m_lastCursor{0};
};

}  //namespace ljn
