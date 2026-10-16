// MapRenderer.cpp - implementation of the tile drawing.

#include "MapRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {
namespace {

//how visible the layers that are not active are, when dimming is on
constexpr float kDimAlpha = 0.35f;

//draws one layer, only the cells that are on screen
void drawLayer(const Map& map, int layer, const Palette& palette, Color tint, int firstX, int firstY, int lastX,
               int lastY) {
    const auto cellSize = static_cast<float>(kCellSize);

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            const TileId id = map.tile(layer, {x, y});
            if (id == kEmptyTile) {
                continue;
            }

            //a big tile covers many cells but must be drawn only once: from its first visible cell
            const Cell origin = map.originOf(layer, {x, y});
            if (x != std::max(origin.x, firstX) || y != std::max(origin.y, firstY)) {
                continue;
            }

            const TileDef& def = map.tileDefs()[static_cast<std::size_t>(id) - 1];
            const Rectangle dest{static_cast<float>(origin.x) * cellSize,
                                 static_cast<float>(origin.y) * cellSize,
                                 static_cast<float>(def.pixelWidth),
                                 static_cast<float>(def.pixelHeight)};
            palette.draw(def, dest, tint);
        }
    }
}

}  //anonymous namespace

void drawTiles(const Map& map, const Palette& palette, const Camera2D& camera, Vector2 viewSize,
               int activeLayer, bool dimOthers) {
    const auto cellSize = static_cast<float>(kCellSize);

    //the cells that are on screen
    const Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
    const Vector2 bottomRight = GetScreenToWorld2D(viewSize, camera);
    const int firstX = std::max(0, static_cast<int>(std::floor(topLeft.x / cellSize)));
    const int firstY = std::max(0, static_cast<int>(std::floor(topLeft.y / cellSize)));
    const int lastX = std::min(map.width() - 1, static_cast<int>(std::floor(bottomRight.x / cellSize)));
    const int lastY = std::min(map.height() - 1, static_cast<int>(std::floor(bottomRight.y / cellSize)));

    //back to front, so the front layers cover the ones behind
    for (int layer = 0; layer < map.layerCount(); ++layer) {
        if (!map.layerInfo(layer).visible) {
            continue;
        }
        const Color tint = (dimOthers && layer != activeLayer) ? Fade(WHITE, kDimAlpha) : WHITE;
        drawLayer(map, layer, palette, tint, firstX, firstY, lastX, lastY);
    }
}

}  //namespace ljn
