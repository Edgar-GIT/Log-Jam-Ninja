// Overlays.hpp - things drawn on top of the tiles: the spawn point, the checkpoints and the problems.

#pragma once

#include <raylib.h>
#include "map/Map.hpp"
#include "validation/Validator.hpp"

namespace ljn {

//a green flag for the spawn point, alpha 1 is solid and lower is see through (the brush preview uses that)
void drawSpawn(Cell cell, float alpha);

//a blue flag for a checkpoint with its number, number 0 draws no text
void drawCheckpoint(Cell cell, int number, float alpha);

//the spawn point and every checkpoint of the map, call it inside BeginMode2D
void drawMarkers(const Map& map);

//a box on every cell that has a problem: red for errors, yellow for warnings, call it inside BeginMode2D
void drawIssues(const ValidationReport& report, float zoom);

}  //namespace ljn
