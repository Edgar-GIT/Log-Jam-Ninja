// Panels.cpp - the side panel: files, undo, tools, the list of tile types and the map check.

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

//space under the list: tile info (86), problems label and button (72) and the hints (96)
constexpr float kFooter = 254.0f;

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

//a button that is grayed out and ignores clicks when it is not enabled
bool button(Rectangle bounds, const char* text, bool enabled = true) {
    if (!enabled) {
        GuiDisable();
    }
    const bool clicked = GuiButton(bounds, text) != 0;
    if (!enabled) {
        GuiEnable();
    }
    return clicked;
}

}  //anonymous namespace

PanelResult SidePanel::draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools,
                            const PanelInfo& info) {
    PanelResult result;
    GuiPanel(area, nullptr);

    const float x = area.x + kPad;
    const float width = area.width - 2.0f * kPad;
    const float half = (width - kGap) * 0.5f;
    const float bottom = area.y + area.height - kPad;
    float y = area.y + kPad;

    //title and the button that opens the import window
    GuiLabel({x, y, width, 28.0f}, "TILES");
    y += 34.0f;
    result.loadTileClicked = button({x, y, width, 40.0f}, "Load tile...");
    y += 48.0f;

    //open and save, the star says there are unsaved changes
    result.openClicked = button({x, y, half, 36.0f}, "Open");
    result.saveClicked = button({x + half + kGap, y, half, 36.0f}, info.dirty ? "Save*" : "Save");
    y += 44.0f;

    //undo and redo, grayed out when there is nothing to do
    result.undoClicked = button({x, y, half, 36.0f}, "Undo", info.canUndo);
    result.redoClicked = button({x + half + kGap, y, half, 36.0f}, "Redo", info.canRedo);
    y += 44.0f;

    //the four tools in two rows, clicking the active one keeps it active
    const auto toolButton = [&](Rectangle bounds, const char* text, Tool tool) {
        bool on = tools.tool == tool;
        GuiToggle(bounds, text, &on);
        if (on) {
            tools.tool = tool;
        }
    };
    toolButton({x, y, half, 36.0f}, "Paint", Tool::Paint);
    toolButton({x + half + kGap, y, half, 36.0f}, "Erase", Tool::Erase);
    y += 42.0f;
    toolButton({x, y, half, 36.0f}, "Spawn", Tool::Spawn);
    toolButton({x + half + kGap, y, half, 36.0f}, "Checkpoint", Tool::Checkpoint);
    y += 44.0f;

    GuiCheckBox({x, y, 28.0f, 28.0f}, "Snap to tile size (G)", &tools.snap);
    y += 40.0f;

    //the list of tile types, in a scrollable area
    const auto defs = map.tileDefs();
    GuiLabel({x, y, width, 24.0f}, std::format("Palette ({})", defs.size()).c_str());
    y += 28.0f;

    const Rectangle list{x, y, width, std::max(96.0f, bottom - kFooter - y)};
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

    //the result of the last check and the button that runs the next one
    const float hintsY = bottom - 88.0f;
    const float checkY = hintsY - 8.0f - 36.0f;
    std::string problems = "Map not checked yet";
    if (info.checked) {
        problems = info.errors + info.warnings == 0
                       ? std::string{"No problems"}
                       : std::format("{} errors, {} warnings", info.errors, info.warnings);
    }
    GuiLabel({x, checkY - 28.0f, width, 24.0f}, problems.c_str());
    result.checkClicked = button({x, checkY, width, 36.0f}, "Check map");

    //hints at the bottom
    GuiLabel({x, hintsY, width, 22.0f}, "LMB place, RMB remove");
    GuiLabel({x, hintsY + 22.0f, width, 22.0f}, "B E S C: pick tool");
    GuiLabel({x, hintsY + 44.0f, width, 22.0f}, "Ctrl+S/O: save, open");
    GuiLabel({x, hintsY + 66.0f, width, 22.0f}, "Ctrl+Z/Y: undo, redo");

    return result;
}

}  //namespace ljn
