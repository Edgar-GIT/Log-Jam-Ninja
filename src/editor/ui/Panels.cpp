// Panels.cpp - the side panel: load button, tools and the list of tile types.

//raygui is one header: this is the only file that compiles its code
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "ui/Panels.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <string>
#include "ui/Text.hpp"

namespace ljn {
namespace {

constexpr float kPad = 12.0f;
constexpr float kThumb = 64.0f;
constexpr float kGap = 6.0f;
constexpr int kColumns = 3;
constexpr float kContentWidth = static_cast<float>(kColumns) * (kThumb + kGap) + kGap;

//space under the list for the info of the selected tile and the hints
constexpr float kFooter = 176.0f;

constexpr Color kThumbBack{40, 42, 56, 255};
constexpr Color kThumbSelected{255, 220, 80, 255};

//where to draw a tile inside its box: as big as fits, in whole multiples when it is scaled up
Rectangle fitInside(const Rectangle& box, const TileDef& def) {
    const float room = box.width - 8.0f;
    float scale = std::min(room / static_cast<float>(def.pixelWidth), room / static_cast<float>(def.pixelHeight));
    if (scale > 1.0f) {
        scale = std::floor(scale);
    }
    const float width = static_cast<float>(def.pixelWidth) * scale;
    const float height = static_cast<float>(def.pixelHeight) * scale;
    return {box.x + (box.width - width) * 0.5f, box.y + (box.height - height) * 0.5f, width, height};
}

}  //anonymous namespace

PanelResult SidePanel::draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools) {
    PanelResult result;
    GuiPanel(area, nullptr);

    const float x = area.x + kPad;
    const float width = area.width - 2.0f * kPad;
    float y = area.y + kPad;

    //title and the button that opens the import window
    GuiLabel({x, y, width, 28.0f}, "TILES");
    y += 34.0f;
    if (GuiButton({x, y, width, 40.0f}, "Load tile...")) {
        result.loadTileClicked = true;
    }
    y += 48.0f;

    //the two tools side by side
    int active = tools.tool == Tool::Paint ? 0 : 1;
    GuiToggleGroup({x, y, (width - 3.0f) * 0.5f, 36.0f}, "Paint (B);Erase (E)", &active);
    tools.tool = active == 0 ? Tool::Paint : Tool::Erase;
    y += 44.0f;

    GuiCheckBox({x, y, 28.0f, 28.0f}, "Snap to tile size (G)", &tools.snap);
    y += 40.0f;

    //the list of tile types, in a scrollable area
    const auto defs = map.tileDefs();
    GuiLabel({x, y, width, 24.0f}, std::format("Palette ({})", defs.size()).c_str());
    y += 28.0f;

    const Rectangle list{x, y, width, std::max(96.0f, area.y + area.height - kPad - kFooter - y)};
    const int rows = (static_cast<int>(defs.size()) + kColumns - 1) / kColumns;
    const Rectangle content{0.0f, 0.0f, kContentWidth, static_cast<float>(rows) * (kThumb + kGap) + kGap};
    Rectangle view{};
    GuiScrollPanel(list, nullptr, content, &m_scroll, &view);

    //the thumbnails, clipped to the visible part of the list
    BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width),
                     static_cast<int>(view.height));
    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < defs.size(); ++i) {
        const int column = static_cast<int>(i) % kColumns;
        const int row = static_cast<int>(i) / kColumns;
        const Rectangle box{list.x + m_scroll.x + kGap + static_cast<float>(column) * (kThumb + kGap),
                            list.y + m_scroll.y + kGap + static_cast<float>(row) * (kThumb + kGap), kThumb,
                            kThumb};
        const auto id = static_cast<TileId>(i + 1);

        DrawRectangleRec(box, kThumbBack);
        palette.draw(defs[i], fitInside(box, defs[i]), WHITE);
        if (id == palette.selected()) {
            DrawRectangleLinesEx(box, 3.0f, kThumbSelected);
        }

        //a click on a thumbnail selects that tile type
        const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !GuiIsLocked()
                          && CheckCollisionPointRec(mouse, box) && CheckCollisionPointRec(mouse, view);
        if (clicked) {
            palette.select(id);
        }
    }
    EndScissorMode();

    //info about the selected tile type
    y = list.y + list.height + 8.0f;
    if (map.hasTile(palette.selected())) {
        const TileDef& def = defs[static_cast<std::size_t>(palette.selected()) - 1];
        const std::string name = shorten(std::filesystem::path(def.file).filename().string(), 20);
        GuiLabel({x, y, width, 24.0f}, std::format("#{} {}", palette.selected(), name).c_str());
        GuiLabel({x, y + 26.0f, width, 24.0f},
                 std::format("{}x{} px  {}", def.pixelWidth, def.pixelHeight, def.solid ? "solid" : "not solid").c_str());
        GuiLabel({x, y + 52.0f, width, 24.0f},
                 std::format("{}x{} cells  from ({}, {})", def.cellsWide(), def.cellsHigh(), def.sourceX, def.sourceY).c_str());
    } else {
        GuiLabel({x, y, width, 24.0f}, "No tile selected");
    }

    //hints at the bottom
    const float hintsY = area.y + area.height - kPad - 66.0f;
    GuiLabel({x, hintsY, width, 22.0f}, "Left click: paint");
    GuiLabel({x, hintsY + 22.0f, width, 22.0f}, "Right click: erase");
    GuiLabel({x, hintsY + 44.0f, width, 22.0f}, "Middle mouse: drag map");

    return result;
}

}  //namespace ljn
