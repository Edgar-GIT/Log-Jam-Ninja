// MapRenderer.hpp - draws the tiles of every layer.

#pragma once

#include <raylib.h>
#include "Palette.hpp"
#include "map/Map.hpp"

namespace ljn {

//draws the visible layers from back to front, without parallax (the editor shows the plain grid)
//when dimOthers is on, every layer except the active one is drawn faded
//when parallax is on, each layer is moved by its parallax values, like the game will do
//call this between BeginMode2D and EndMode2D, after drawGrid
void drawTiles(const Map& map, const Palette& palette, const Camera2D& camera, Vector2 viewSize,
               int activeLayer, bool dimOthers, bool parallax);

}  //namespace ljn
