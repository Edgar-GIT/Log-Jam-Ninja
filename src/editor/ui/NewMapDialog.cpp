// NewMapDialog.cpp - the new map window: two numbers and two buttons.

#include "ui/NewMapDialog.hpp"

#include <algorithm>
#include <format>
#include <raygui.h>
#include "map/Map.hpp"

namespace ljn {
namespace {

//smallest sizes that make sense for a level, the largest come from the Map class
constexpr int kMinWidth = 16;
constexpr int kMinHeight = 8;

constexpr Color kShade{0, 0, 0, 170};

}  //anonymous namespace

std::optional<MapSize> NewMapDialog::draw(Vector2 screenSize) {
    if (!m_open) {
        return std::nullopt;
    }

    //dim everything behind the window
    DrawRectangle(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), kShade);

    //the window, centered
    const Rectangle window{(screenSize.x - 420.0f) * 0.5f, (screenSize.y - 300.0f) * 0.5f, 420.0f, 300.0f};
    if (GuiWindowBox(window, "New map") != 0 || IsKeyPressed(KEY_ESCAPE)) {
        m_open = false;
        m_editWidth = false;
        m_editHeight = false;
        return std::nullopt;
    }

    const float x = window.x + 20.0f;
    const float width = window.width - 40.0f;
    float y = window.y + 24.0f + 20.0f;

    //width and height in cells
    GuiLabel({x, y, width, 26.0f}, std::format("Width in cells ({} to {})", kMinWidth, Map::kMaxWidth).c_str());
    y += 28.0f;
    if (GuiSpinner({x, y, width, 36.0f}, nullptr, &m_width, kMinWidth, Map::kMaxWidth, m_editWidth)) {
        m_editWidth = !m_editWidth;
    }
    y += 48.0f;
    GuiLabel({x, y, width, 26.0f}, std::format("Height in cells ({} to {})", kMinHeight, Map::kMaxHeight).c_str());
    y += 28.0f;
    if (GuiSpinner({x, y, width, 36.0f}, nullptr, &m_height, kMinHeight, Map::kMaxHeight, m_editHeight)) {
        m_editHeight = !m_editHeight;
    }
    y += 44.0f;

    //the size in pixels of the game world, so it is easy to picture
    GuiLabel({x, y, width, 26.0f}, std::format("{} x {} pixels", m_width * kCellSize, m_height * kCellSize).c_str());

    //buttons at the bottom
    const float half = (width - 8.0f) * 0.5f;
    const float buttonY = window.y + window.height - 56.0f;
    if (GuiButton({x, buttonY, half, 40.0f}, "Create")) {
        m_open = false;
        m_editWidth = false;
        m_editHeight = false;
        return MapSize{std::clamp(m_width, kMinWidth, Map::kMaxWidth), std::clamp(m_height, kMinHeight, Map::kMaxHeight)};
    }
    if (GuiButton({x + half + 8.0f, buttonY, half, 40.0f}, "Cancel")) {
        m_open = false;
        m_editWidth = false;
        m_editHeight = false;
    }
    return std::nullopt;
}

}  //namespace ljn
