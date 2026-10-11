// MapRenderer.cpp - implementation of the tile drawing.

#include "MapRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {

void drawTiles(const Map& map, const Palette& palette, const Camera2D& camera, Vector2 viewSize) {
    const auto cellSize = static_cast<float>(kCellSize);

    //the cells that are on screen
    const Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
    const Vector2 bottomRight = GetScreenToWorld2D(viewSize, camera);
    const int firstX = std::max(0, static_cast<int>(std::floor(topLeft.x / cellSize)));
    const int firstY = std::max(0, static_cast<int>(std::floor(topLeft.y / cellSize)));
    const int lastX = std::min(map.width() - 1, static_cast<int>(std::floor(bottomRight.x / cellSize)));
    const int lastY = std::min(map.height() - 1, static_cast<int>(std::floor(bottomRight.y / cellSize)));

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            const TileId id = map.tile({x, y});
            if (id == kEmptyTile) {
                continue;
            }

            //a big tile covers many cells but must be drawn only once: from its first visible cell
            const Cell origin = map.originOf({x, y});
            if (x != std::max(origin.x, firstX) || y != std::max(origin.y, firstY)) {
                continue;
            }

            const TileDef& def = map.tileDefs()[static_cast<std::size_t>(id) - 1];
            const Rectangle dest{static_cast<float>(origin.x) * cellSize,
                                 static_cast<float>(origin.y) * cellSize,
                                 static_cast<float>(def.pixelWidth),
                                 static_cast<float>(def.pixelHeight)};
            palette.draw(def, dest, WHITE);
        }
    }
}

}  //namespace ljn
