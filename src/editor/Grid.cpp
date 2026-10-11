// Grid.cpp - implementation of the grid drawing.

#include "Grid.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {
namespace {

constexpr Color kMapColor{32, 34, 48, 255};
constexpr Color kLineColor{255, 255, 255, 28};
constexpr Color kBorderColor{255, 255, 255, 140};

//grid lines are hidden when a cell is smaller than this on screen, they would just be noise
constexpr float kMinCellOnScreen = 8.0f;

}  //anonymous namespace

void drawGrid(const Map& map, const Camera2D& camera, Vector2 viewSize) {
    const auto cell = static_cast<float>(kCellSize);
    const auto mapWidth = static_cast<float>(map.width()) * cell;
    const auto mapHeight = static_cast<float>(map.height()) * cell;

    //the map area
    DrawRectangleRec({0.0f, 0.0f, mapWidth, mapHeight}, kMapColor);

    //which cells are visible, so we only draw those lines
    const Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
    const Vector2 bottomRight = GetScreenToWorld2D(viewSize, camera);
    const int firstX = std::max(0, static_cast<int>(std::floor(topLeft.x / cell)));
    const int firstY = std::max(0, static_cast<int>(std::floor(topLeft.y / cell)));
    const int lastX = std::min(map.width(), static_cast<int>(std::ceil(bottomRight.x / cell)));
    const int lastY = std::min(map.height(), static_cast<int>(std::ceil(bottomRight.y / cell)));

    //the grid lines, always 1 pixel thick on screen
    if (cell * camera.zoom >= kMinCellOnScreen) {
        const float thickness = 1.0f / camera.zoom;
        const float top = static_cast<float>(firstY) * cell;
        const float bottom = static_cast<float>(lastY) * cell;
        const float left = static_cast<float>(firstX) * cell;
        const float right = static_cast<float>(lastX) * cell;

        for (int x = firstX; x <= lastX; ++x) {
            const float px = static_cast<float>(x) * cell;
            DrawLineEx({px, top}, {px, bottom}, thickness, kLineColor);
        }
        for (int y = firstY; y <= lastY; ++y) {
            const float py = static_cast<float>(y) * cell;
            DrawLineEx({left, py}, {right, py}, thickness, kLineColor);
        }
    }

    //the map border, 2 pixels thick on screen
    DrawRectangleLinesEx({0.0f, 0.0f, mapWidth, mapHeight}, 2.0f / camera.zoom, kBorderColor);
}

}  //namespace ljn
