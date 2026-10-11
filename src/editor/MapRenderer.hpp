// MapRenderer.hpp - draws the tiles that are placed on the map.

#pragma once

#include <raylib.h>
#include "Palette.hpp"
#include "map/Map.hpp"

namespace ljn {

//call this between BeginMode2D and EndMode2D, after drawGrid
void drawTiles(const Map& map, const Palette& palette, const Camera2D& camera, Vector2 viewSize);

}  //namespace ljn
