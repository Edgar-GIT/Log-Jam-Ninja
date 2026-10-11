// Grid.hpp - draws the map area, the grid lines and the map border.

#pragma once

#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

//call this between BeginMode2D and EndMode2D
void drawGrid(const Map& map, const Camera2D& camera, Vector2 viewSize);

}  //namespace ljn
