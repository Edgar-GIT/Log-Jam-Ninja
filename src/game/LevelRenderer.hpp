// LevelRenderer.hpp - draws a level into the 960x540 picture, with parallax and no blur.

#pragma once

#include <raylib.h>
#include "TileSet.hpp"
#include "map/Map.hpp"

namespace ljn {

//the camera is the top-left corner of what the player sees, in world pixels
//every visible layer is drawn back to front, moved by the camera times its parallax, in whole pixels
void drawLevel(const Map& map, const TileSet& tiles, Vector2 camera, int viewWidth, int viewHeight);

//the same, but only the layers from first to last (both included), so the player can be drawn between two groups
void drawLayers(const Map& map, const TileSet& tiles, Vector2 camera, int viewWidth, int viewHeight, int first,
                int last);

//the layer the player is drawn on top of: the last solid layer (the front layers cover the player)
[[nodiscard]] int playerLayer(const Map& map);

//the camera position that keeps the view inside the map (a map smaller than the view stays at 0)
[[nodiscard]] Vector2 clampCamera(const Map& map, Vector2 camera, int viewWidth, int viewHeight);

}  //namespace ljn
