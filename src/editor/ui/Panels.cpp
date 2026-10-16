// Panels.cpp - the side panel: files, undo, tools, and two tabs (tile types and layers) and the map check.

//raygui is one header: this is the only file that compiles its code
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "ui/Panels.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
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

//space under the content of a tab: the check label (28), the check button (36) and some air (8)
constexpr float kBottomArea = 72.0f;

//the tile info under the list: three lines
constexpr float kTileInfo = 78.0f;

//layer list: height of the box and of one row
constexpr float kLayerListHeight = 130.0f;
constexpr float kLayerRow = 26.0f;
constexpr float kLayerBox = 24.0f;

//how much one click on the parallax buttons changes the value, and the allowed range
constexpr float kParallaxStep = 0.1f;
constexpr float kParallaxMax = 4.0f;

constexpr Color kThumbBack{40, 42, 56, 255};
constexpr Color kThumbSelected{255, 220, 80, 255};
constexpr Color kRowBack{40, 42, 56, 255};
constexpr Color kRowActive{70, 74, 100, 255};
constexpr Color kBoxOn{70, 160, 90, 255};
constexpr Color kBoxOff{60, 60, 70, 255};
constexpr Color kRowText{235, 235, 240, 255};
constexpr Color kRowTextHidden{140, 140, 150, 255};

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

//a small square with a letter, green when on and gray when off (used for visible and solid in the layer list)
void drawLetterBox(Rectangle box, const char* letter, bool on) {
    DrawRectangleRec(box, on ? kBoxOn : kBoxOff);
    DrawRectangleLinesEx(box, 1.0f, kRowText);
    const int width = MeasureText(letter, 18);
    DrawText(letter, static_cast<int>(box.x + (box.width - static_cast<float>(width)) * 0.5f),
             static_cast<int>(box.y + 3.0f), 18, on ? kRowText : kRowTextHidden);
}

//rounds a parallax value to two decimals and keeps it in the allowed range
float cleanParallax(float value) {
    return std::clamp(std::round(value * 100.0f) / 100.0f, 0.0f, kParallaxMax);
}

//a row with a label, a minus button, the value and a plus button, returns the new value or nothing
std::optional<float> parallaxRow(float x, float y, const char* label, float value) {
    GuiLabel({x, y, 110.0f, 30.0f}, label);
    std::optional<float> result;
    if (button({x + 110.0f, y, 34.0f, 30.0f}, "-", value > 0.0f)) {
        result = cleanParallax(value - kParallaxStep);
    }
    GuiLabel({x + 152.0f, y, 56.0f, 30.0f}, std::format("{:.2f}", value).c_str());
    if (button({x + 210.0f, y, 34.0f, 30.0f}, "+", value < kParallaxMax)) {
        result = cleanParallax(value + kParallaxStep);
    }
    return result;
}

}  //anonymous namespace

PanelResult SidePanel::draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools,
                            const PanelInfo& info) {
    PanelResult result;
    GuiPanel(area, nullptr);

    //the active layer must always exist, also after undo, open or removing a layer
    tools.layer = std::clamp(tools.layer, 0, std::max(0, map.layerCount() - 1));

    const float x = area.x + kPad;
    const float width = area.width - 2.0f * kPad;
    const float half = (width - kGap) * 0.5f;
    const float bottom = area.y + area.height - kPad;
    float y = area.y + kPad;

    //the two tabs
    int tab = static_cast<int>(m_tab);
    GuiToggleGroup({x, y, half, 32.0f}, "Tiles;Layers", &tab);
    m_tab = static_cast<Tab>(tab);
    y += 40.0f;

    //new, open and save, the star says there are unsaved changes
    const float third = (width - 2.0f * kGap) / 3.0f;
    result.newClicked = button({x, y, third, 36.0f}, "New");
    result.openClicked = button({x + third + kGap, y, third, 36.0f}, "Open");
    result.saveClicked = button({x + 2.0f * (third + kGap), y, third, 36.0f}, info.dirty ? "Save*" : "Save");
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

    //what is inside the tab
    const float contentBottom = bottom - kBottomArea;
    if (m_tab == Tab::Tiles) {
        drawTilesTab(x, y, width, contentBottom, map, palette, result);
    } else {
        drawLayersTab(x, y, width, contentBottom, map, tools, result);
    }

    //the result of the last check and the button that runs the next one
    const float checkY = bottom - 36.0f;
    std::string problems = "Map not checked yet";
    if (info.checked) {
        problems = info.errors + info.warnings == 0
                       ? std::string{"No problems"}
                       : std::format("{} errors, {} warnings", info.errors, info.warnings);
    }
    GuiLabel({x, checkY - 28.0f, width, 24.0f}, problems.c_str());
    result.checkClicked = button({x, checkY, width, 36.0f}, "Check map");

    return result;
}

void SidePanel::drawTilesTab(float x, float y, float width, float bottom, const Map& map, Palette& palette,
                             PanelResult& result) {
    //the two buttons that open the import windows: a still tile or an animated one
    const float half = (width - kGap) * 0.5f;
    result.loadTileClicked = button({x, y, half, 40.0f}, "Load tile");
    result.loadAnimatedClicked = button({x + half + kGap, y, half, 40.0f}, "Animated");
    y += 48.0f;

    //the list of tile types, in a scrollable area
    const auto defs = map.tileDefs();
    GuiLabel({x, y, width, 24.0f}, std::format("Palette ({})", defs.size()).c_str());
    y += 28.0f;

    const Rectangle list{x, y, width, std::max(96.0f, bottom - kTileInfo - 8.0f - y)};
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
        if (def.isAnimated()) {
            GuiLabel({x, y + 52.0f, width, 24.0f},
                     std::format("{} frames, {} ms each", def.frames.size(), def.frameMs).c_str());
        } else {
            GuiLabel({x, y + 52.0f, width, 24.0f},
                     std::format("{}x{} cells  from ({}, {})", def.cellsWide(), def.cellsHigh(), def.sourceX, def.sourceY).c_str());
        }
    } else {
        GuiLabel({x, y, width, 24.0f}, "No tile selected");
    }
}

void SidePanel::drawLayersTab(float x, float y, float width, float /*bottom*/, const Map& map, ToolState& tools,
                              PanelResult& result) {
    const int count = map.layerCount();
    GuiLabel({x, y, width, 24.0f}, std::format("Layers ({})  V show  S solid", count).c_str());
    y += 28.0f;

    //the list, the front layer is the first row
    const Rectangle list{x, y, width, kLayerListHeight};
    const float contentWidth = width - 20.0f;
    const Rectangle content{0.0f, 0.0f, contentWidth, static_cast<float>(count) * kLayerRow + 2.0f};
    Rectangle view{};
    GuiScrollPanel(list, nullptr, content, &m_layerScroll, &view);

    BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width),
                     static_cast<int>(view.height));
    const Vector2 mouse = GetMousePosition();
    const bool mayClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !GuiIsLocked() && CheckCollisionPointRec(mouse, view);
    for (int row = 0; row < count; ++row) {
        const int layer = count - 1 - row;
        const LayerInfo& layerInfo = map.layerInfo(layer);
        const Rectangle box{list.x + m_layerScroll.x + 2.0f,
                            list.y + m_layerScroll.y + 2.0f + static_cast<float>(row) * kLayerRow,
                            contentWidth - 4.0f, kLayerRow - 2.0f};
        const Rectangle visibleBox{box.x + 2.0f, box.y + 1.0f, kLayerBox, kLayerBox};
        const Rectangle solidBox{box.x + 2.0f + kLayerBox + 4.0f, box.y + 1.0f, kLayerBox, kLayerBox};

        //the row: lighter and outlined when it is the active layer
        const bool active = layer == tools.layer;
        DrawRectangleRec(box, active ? kRowActive : kRowBack);
        if (active) {
            DrawRectangleLinesEx(box, 2.0f, kThumbSelected);
        }
        drawLetterBox(visibleBox, "V", layerInfo.visible);
        drawLetterBox(solidBox, "S", layerInfo.solid);
        DrawText(shorten(layerInfo.name, 14).c_str(), static_cast<int>(solidBox.x + kLayerBox + 8.0f),
                 static_cast<int>(box.y + 3.0f), 18, layerInfo.visible ? kRowText : kRowTextHidden);

        //a click on a box flips that setting, a click anywhere else on the row selects the layer
        if (mayClick && CheckCollisionPointRec(mouse, box)) {
            LayerInfo changed = layerInfo;
            if (CheckCollisionPointRec(mouse, visibleBox)) {
                changed.visible = !changed.visible;
                result.layerEdit = LayerEdit{layer, std::move(changed)};
            } else if (CheckCollisionPointRec(mouse, solidBox)) {
                changed.solid = !changed.solid;
                result.layerEdit = LayerEdit{layer, std::move(changed)};
            } else {
                tools.layer = layer;
            }
        }
    }
    EndScissorMode();
    y += kLayerListHeight + 8.0f;

    //add, remove, and move to the front or the back
    const float quarter = (width - 3.0f * kGap) * 0.25f;
    result.addLayerClicked = button({x, y, quarter, 36.0f}, "+", count < Map::kMaxLayers);
    result.removeLayerClicked = button({x + (quarter + kGap), y, quarter, 36.0f}, "-", count > 1);
    if (button({x + 2.0f * (quarter + kGap), y, quarter, 36.0f}, "Up", tools.layer < count - 1)) {
        result.moveLayer = 1;
    }
    if (button({x + 3.0f * (quarter + kGap), y, quarter, 36.0f}, "Down", tools.layer > 0)) {
        result.moveLayer = -1;
    }
    y += 44.0f;

    if (!map.hasLayer(tools.layer)) {
        return;
    }
    const LayerInfo& current = map.layerInfo(tools.layer);

    //the name: while it is not being edited the box shows the real name of the active layer
    if (!m_nameEditing) {
        std::strncpy(m_nameBuffer, current.name.c_str(), sizeof(m_nameBuffer) - 1);
        m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
        m_nameLayer = tools.layer;
    }
    if (GuiTextBox({x, y, width, 30.0f}, m_nameBuffer, static_cast<int>(sizeof(m_nameBuffer)), m_nameEditing) != 0) {
        //the box was clicked or Enter was pressed: finishing the edit applies the new name
        if (m_nameEditing && m_nameBuffer[0] != '\0' && map.hasLayer(m_nameLayer)) {
            LayerInfo renamed = map.layerInfo(m_nameLayer);
            renamed.name = m_nameBuffer;
            result.layerEdit = LayerEdit{m_nameLayer, std::move(renamed)};
        }
        m_nameEditing = !m_nameEditing;
    }
    y += 38.0f;

    //parallax: how much the layer follows the camera (the game uses it, the editor only stores it)
    if (const auto value = parallaxRow(x, y, "Parallax X", current.parallaxX)) {
        LayerInfo changed = current;
        changed.parallaxX = *value;
        result.layerEdit = LayerEdit{tools.layer, std::move(changed)};
    }
    y += 36.0f;
    if (const auto value = parallaxRow(x, y, "Parallax Y", current.parallaxY)) {
        LayerInfo changed = current;
        changed.parallaxY = *value;
        result.layerEdit = LayerEdit{tools.layer, std::move(changed)};
    }
    y += 40.0f;

    GuiCheckBox({x, y, 28.0f, 28.0f}, "Dim other layers", &tools.dimOthers);
    y += 34.0f;
    GuiCheckBox({x, y, 28.0f, 28.0f}, "Parallax preview (P)", &tools.parallaxPreview);
}

}  //namespace ljn
