// MenuView.cpp - implementation of the menu drawing.

#include "MenuView.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include "Style.hpp"

namespace ljn {
namespace {

//the height of a row and the size of the text
constexpr float kRowHeight = 32.0f;
constexpr int kTextSize = style::kItemSize;

//the width of the bar of a slider
constexpr float kSliderWidth = 150.0f;

//the space between two letters of the menu font
constexpr float kFontSpacing = 1.0f;

//the width of a text, with the menu font when there is one
float textWidth(const Font* font, const std::string& text, int size) {
    if (font != nullptr) {
        return MeasureTextEx(*font, text.c_str(), static_cast<float>(size), kFontSpacing).x;
    }
    return static_cast<float>(MeasureText(text.c_str(), size));
}

//a text at x, y, with the menu font when there is one
void drawText(const Font* font, const std::string& text, float x, float y, int size, Color color) {
    if (font != nullptr) {
        DrawTextEx(*font, text.c_str(), Vector2{x, y}, static_cast<float>(size), kFontSpacing, color);
        return;
    }
    DrawText(text.c_str(), static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), size, color);
}

//draws a text with its right side at rightX
void drawRight(const Font* font, const std::string& text, float rightX, float y, int size, Color color) {
    drawText(font, text, rightX - textWidth(font, text, size), y, size, color);
}

}  //anonymous namespace

Rectangle MenuView::rowRect(const Menu& menu, int index, const Rectangle& area) noexcept {
    const int count = static_cast<int>(menu.size());
    const float height = count > 0 ? std::min(kRowHeight, area.height / static_cast<float>(count)) : kRowHeight;
    return {area.x, area.y + static_cast<float>(index) * height, area.width, height};
}

Rectangle MenuView::sliderRect(const Rectangle& row) noexcept {
    return {row.x + row.width - kSliderWidth, row.y + (row.height - 8.0f) * 0.5f, kSliderWidth, 8.0f};
}

void MenuView::draw(const Menu& menu, const Rectangle& area) const {
    draw(menu, area, nullptr);
}

void MenuView::draw(const Menu& menu, const Rectangle& area, const Font& font) const {
    draw(menu, area, &font);
}

void MenuView::draw(const Menu& menu, const Rectangle& area, const Font* font) const {
    for (std::size_t i = 0; i < menu.size(); ++i) {
        const MenuItem& item = menu.items()[i];
        const Rectangle row = rowRect(menu, static_cast<int>(i), area);
        const bool selected = static_cast<int>(i) == menu.cursor();

        if (selected) {
            DrawRectangleRec(row, style::kHighlight);
        }

        //the items that cannot be used are dim
        const Color color = !item.enabled ? style::kTextDim : (selected ? style::kAccent : style::kText);
        const float textY = std::round(row.y + (row.height - kTextSize) * 0.5f);
        const float rightX = row.x + row.width - 12.0f;

        if (item.kind == ItemKind::Label) {
            drawText(font, item.label, row.x, textY, kTextSize, style::kTextDim);
            continue;
        }

        drawText(font, item.label, row.x + 10.0f, textY, kTextSize, color);

        switch (item.kind) {
            case ItemKind::Toggle: {
                drawRight(font, item.value != 0 ? "On" : "Off", rightX, textY, kTextSize, color);
                break;
            }
            case ItemKind::Slider: {
                //the number on the left of the bar
                const Rectangle bar = sliderRect(row);
                drawRight(font, std::to_string(item.value), bar.x - 10.0f, textY, kTextSize, color);

                //the bar: the empty part and the part that is filled
                DrawRectangleRec(bar, style::kPanelLine);
                const float span = static_cast<float>(item.max - item.min);
                const float filled = span > 0.0f ? static_cast<float>(item.value - item.min) / span : 0.0f;
                DrawRectangleRec({bar.x, bar.y, bar.width * filled, bar.height}, style::kAccent);
                break;
            }
            case ItemKind::Choice: {
                const std::string text =
                    item.value >= 0 && static_cast<std::size_t>(item.value) < item.choices.size()
                        ? item.choices[static_cast<std::size_t>(item.value)]
                        : std::string{};
                drawRight(font, text, rightX, textY, kTextSize, color);
                break;
            }
            case ItemKind::Button: {
                drawRight(font, item.detail, rightX, textY, kTextSize, style::kTextDim);
                break;
            }
            case ItemKind::Label:
                break;
        }
    }
}

void MenuView::draw(const Menu& menu) const {
    draw(menu, m_area);
}

MenuEvent MenuView::updateMouse(Menu& menu, Vector2 mouse, bool moved, bool pressed, const Rectangle& area) {
    const int index = itemAt(menu, mouse, area);
    if (index < 0) {
        return {};
    }

    //the item under the mouse becomes the one under the cursor
    if (moved) {
        menu.hover(index);
    }
    if (pressed) {
        return menu.click(index, sliderFraction(menu, index, mouse, area));
    }
    return {};
}

MenuEvent MenuView::updateMouse(Menu& menu, Vector2 mouse, bool moved, bool pressed) {
    return updateMouse(menu, mouse, moved, pressed, m_area);
}

int MenuView::itemAt(const Menu& menu, Vector2 position, const Rectangle& area) const {
    for (std::size_t i = 0; i < menu.size(); ++i) {
        //the titles are not things to click
        if (menu.items()[i].kind == ItemKind::Label) {
            continue;
        }
        const Rectangle row = rowRect(menu, static_cast<int>(i), area);
        if (CheckCollisionPointRec(position, row)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float MenuView::sliderFraction(const Menu& menu, int index, Vector2 position, const Rectangle& area) const {
    if (index < 0 || index >= static_cast<int>(menu.size())) {
        return 0.0f;
    }
    const Rectangle bar = sliderRect(rowRect(menu, index, area));
    if (bar.width <= 0.0f) {
        return 0.0f;
    }
    return std::clamp((position.x - bar.x) / bar.width, 0.0f, 1.0f);
}

}  //namespace ljn
