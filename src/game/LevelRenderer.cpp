// LevelRenderer.cpp - implementation of the level drawing.

#include "LevelRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {
namespace {

//draws one layer shifted by (offsetX, offsetY) pixels, only the cells that are on screen
void drawLayer(const Map& map, int layer, const TileSet& tiles, int offsetX, int offsetY, int viewWidth,
               int viewHeight) {
    //the cells that are inside the view (the offset is never negative, the camera is clamped)
    const int firstX = std::max(0, offsetX / kCellSize);
    const int firstY = std::max(0, offsetY / kCellSize);
    const int lastX = std::min(map.width() - 1, (offsetX + viewWidth) / kCellSize);
    const int lastY = std::min(map.height() - 1, (offsetY + viewHeight) / kCellSize);

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            const TileId id = map.tile(layer, {x, y});
            if (id == kEmptyTile) {
                continue;
            }

            //a big tile covers many cells but is drawn once, from its first visible cell
            const Cell origin = map.originOf(layer, {x, y});
            if (x != std::max(origin.x, firstX) || y != std::max(origin.y, firstY)) {
                continue;
            }

            const TileDef& def = map.tileDefs()[static_cast<std::size_t>(id) - 1];
            const Rectangle dest{static_cast<float>(origin.x * kCellSize - offsetX),
                                 static_cast<float>(origin.y * kCellSize - offsetY),
                                 static_cast<float>(def.pixelWidth), static_cast<float>(def.pixelHeight)};
            tiles.draw(def, dest);
        }
    }
}

}  //anonymous namespace

void drawLevel(const Map& map, const TileSet& tiles, Vector2 camera, int viewWidth, int viewHeight) {
    drawLayers(map, tiles, camera, viewWidth, viewHeight, 0, map.layerCount() - 1);
}

void drawLayers(const Map& map, const TileSet& tiles, Vector2 camera, int viewWidth, int viewHeight, int first,
                int last) {
    //back to front, so the front layers cover the ones behind
    for (int layer = std::max(first, 0); layer <= std::min(last, map.layerCount() - 1); ++layer) {
        const LayerInfo& info = map.layerInfo(layer);
        if (!info.visible) {
            continue;
        }

        //whole pixels only: a half pixel offset would blur or shake the pixel art
        const auto offsetX = static_cast<int>(std::floor(camera.x * info.parallaxX));
        const auto offsetY = static_cast<int>(std::floor(camera.y * info.parallaxY));
        drawLayer(map, layer, tiles, offsetX, offsetY, viewWidth, viewHeight);
    }
}

int playerLayer(const Map& map) {
    for (int layer = map.layerCount() - 1; layer >= 0; --layer) {
        if (map.layerInfo(layer).solid) {
            return layer;
        }
    }

    //no solid layer: the player is in front of everything
    return map.layerCount() - 1;
}

Vector2 clampCamera(const Map& map, Vector2 camera, int viewWidth, int viewHeight) {
    const float maxX = std::max(0.0f, static_cast<float>(map.width() * kCellSize - viewWidth));
    const float maxY = std::max(0.0f, static_cast<float>(map.height() * kCellSize - viewHeight));
    return {std::clamp(camera.x, 0.0f, maxX), std::clamp(camera.y, 0.0f, maxY)};
}

}  //namespace ljn
