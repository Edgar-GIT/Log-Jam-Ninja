// Tools.cpp - implementation of the brush: painting, erasing and the preview.

#include "Tools.hpp"

namespace ljn {
namespace {

constexpr Color kHoverFill{255, 255, 255, 40};
constexpr Color kHoverLine{255, 220, 80, 255};
constexpr Color kGhostTint{190, 190, 190, 140};
constexpr Color kBlockedFill{255, 60, 60, 110};
constexpr Color kBlockedLine{255, 70, 70, 255};
constexpr Color kEraseFill{255, 60, 60, 70};

//rectangle of a group of cells, in map pixels
Rectangle cellsRect(Cell origin, int cellsWide, int cellsHigh) {
    const auto size = static_cast<float>(kCellSize);
    return {static_cast<float>(origin.x) * size, static_cast<float>(origin.y) * size,
            static_cast<float>(cellsWide) * size, static_cast<float>(cellsHigh) * size};
}

//moves a cell back to the grid of the tile's own size, so a 2x2 tile only lands on even cells
Cell snapped(Cell cell, const TileDef& def) {
    return {cell.x - cell.x % def.cellsWide(), cell.y - cell.y % def.cellsHigh()};
}

//the tile type selected in the palette, or nullptr if there is none
const TileDef* selectedDef(const Map& map, const Palette& palette) {
    if (!map.hasTile(palette.selected())) {
        return nullptr;
    }
    return &map.tileDefs()[static_cast<std::size_t>(palette.selected()) - 1];
}

//the plain highlight of one cell
void drawHoverCell(Cell cell, float zoom) {
    const Rectangle rect = cellsRect(cell, 1, 1);
    DrawRectangleRec(rect, kHoverFill);
    DrawRectangleLinesEx(rect, 2.0f / zoom, kHoverLine);
}

}  //anonymous namespace

void Brush::update(Map& map, const Palette& palette, const ToolState& tools, const EditorCamera& camera,
                   Vector2 viewSize, bool inputEnabled) {
    m_hovered.reset();
    if (!inputEnabled) {
        m_stroke = Stroke::None;
        return;
    }

    //the cell under the mouse, only if the mouse is over the map view
    const Vector2 mouse = GetMousePosition();
    const bool inView = mouse.x >= 0.0f && mouse.y >= 0.0f && mouse.x < viewSize.x && mouse.y < viewSize.y;
    if (inView) {
        m_hovered = camera.cellAt(mouse, map);
    }

    //a stroke starts with a click on the map and lasts until every button is released
    if (inView && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_stroke = tools.tool == Tool::Paint ? Stroke::Paint : Stroke::Erase;
    } else if (inView && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        m_stroke = Stroke::Erase;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        m_stroke = Stroke::None;
    }
    if (!m_hovered) {
        return;
    }

    //apply the stroke to the cell under the mouse
    if (m_stroke == Stroke::Erase) {
        map.erase(*m_hovered);
    } else if (m_stroke == Stroke::Paint) {
        if (const TileDef* def = selectedDef(map, palette)) {
            const Cell origin = tools.snap ? snapped(*m_hovered, *def) : *m_hovered;
            map.place(origin, palette.selected());
        }
    }
}

void Brush::drawPreview(const Map& map, const Palette& palette, const ToolState& tools, float zoom) const {
    if (!m_hovered) {
        return;
    }

    //erasing (erase tool, or right button): show the whole tile that would be removed
    if (tools.tool == Tool::Erase || m_stroke == Stroke::Erase) {
        const TileId id = map.tile(*m_hovered);
        if (id == kEmptyTile) {
            drawHoverCell(*m_hovered, zoom);
            return;
        }
        const TileDef& def = map.tileDefs()[static_cast<std::size_t>(id) - 1];
        const Rectangle rect = cellsRect(map.originOf(*m_hovered), def.cellsWide(), def.cellsHigh());
        DrawRectangleRec(rect, kEraseFill);
        DrawRectangleLinesEx(rect, 2.0f / zoom, kBlockedLine);
        return;
    }

    //painting with nothing selected: just highlight the cell
    const TileDef* def = selectedDef(map, palette);
    if (def == nullptr) {
        drawHoverCell(*m_hovered, zoom);
        return;
    }

    //the tile as a see through gray ghost, where it would land
    const Cell origin = tools.snap ? snapped(*m_hovered, *def) : *m_hovered;
    const auto cellSize = static_cast<float>(kCellSize);
    const Rectangle sprite{static_cast<float>(origin.x) * cellSize, static_cast<float>(origin.y) * cellSize,
                           static_cast<float>(def->pixelWidth), static_cast<float>(def->pixelHeight)};
    palette.draw(*def, sprite, kGhostTint);

    //every cell of the footprint that is taken or outside the map turns red
    for (int dy = 0; dy < def->cellsHigh(); ++dy) {
        for (int dx = 0; dx < def->cellsWide(); ++dx) {
            const Cell cell{origin.x + dx, origin.y + dy};
            if (!map.isFree(cell)) {
                DrawRectangleRec(cellsRect(cell, 1, 1), kBlockedFill);
            }
        }
    }

    //outline of the whole footprint: yellow if it fits, red if it does not
    const bool fits = map.canPlace(origin, palette.selected());
    DrawRectangleLinesEx(cellsRect(origin, def->cellsWide(), def->cellsHigh()), 2.0f / zoom,
                         fits ? kHoverLine : kBlockedLine);
}

}  //namespace ljn
