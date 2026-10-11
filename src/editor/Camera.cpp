// Camera.cpp - implementation of the editor camera.

#include "Camera.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {
namespace {

//how fast the arrow keys move the view, in screen pixels per second
constexpr float kPanSpeed = 900.0f;

//keeps one axis of the view inside the map, or centers it if the map is smaller than the view
float clampAxis(float position, float mapSize, float viewSize) {
    if (mapSize <= viewSize) {
        return (mapSize - viewSize) * 0.5f;
    }
    return std::clamp(position, 0.0f, mapSize - viewSize);
}

}  //anonymous namespace

void EditorCamera::update(float dt, Vector2 viewSize, const Map& map) {
    const Vector2 mouse = GetMousePosition();
    const bool mouseInView =
        mouse.x >= 0.0f && mouse.y >= 0.0f && mouse.x < viewSize.x && mouse.y < viewSize.y;

    //zoom with the mouse wheel, the map point under the mouse stays where it is
    const float wheel = GetMouseWheelMove();
    if (mouseInView && wheel != 0.0f) {
        const Vector2 worldBefore = GetScreenToWorld2D(mouse, m_camera);
        if (wheel > 0.0f && m_zoomIndex + 1 < kZoomLevels.size()) {
            ++m_zoomIndex;
        } else if (wheel < 0.0f && m_zoomIndex > 0) {
            --m_zoomIndex;
        }
        m_camera.zoom = kZoomLevels[m_zoomIndex];
        m_camera.target = {worldBefore.x - mouse.x / m_camera.zoom,
                           worldBefore.y - mouse.y / m_camera.zoom};
    }

    //move with the arrow keys, the speed on screen is the same at any zoom
    const float step = kPanSpeed / m_camera.zoom * dt;
    if (IsKeyDown(KEY_LEFT)) {
        m_camera.target.x -= step;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        m_camera.target.x += step;
    }
    if (IsKeyDown(KEY_UP)) {
        m_camera.target.y -= step;
    }
    if (IsKeyDown(KEY_DOWN)) {
        m_camera.target.y += step;
    }

    //move by dragging with the middle mouse button
    if (mouseInView && IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        const Vector2 drag = GetMouseDelta();
        m_camera.target.x -= drag.x / m_camera.zoom;
        m_camera.target.y -= drag.y / m_camera.zoom;
    }

    //keep the view inside the map, then snap it to whole screen pixels
    const float zoom = m_camera.zoom;
    const auto mapWidth = static_cast<float>(map.width() * kCellSize);
    const auto mapHeight = static_cast<float>(map.height() * kCellSize);
    const float x = clampAxis(m_camera.target.x, mapWidth, viewSize.x / zoom);
    const float y = clampAxis(m_camera.target.y, mapHeight, viewSize.y / zoom);
    m_camera.target = {std::round(x * zoom) / zoom, std::round(y * zoom) / zoom};
}

void EditorCamera::jumpToBottomLeft(Vector2 viewSize, const Map& map) {
    const auto mapHeight = static_cast<float>(map.height() * kCellSize);
    m_camera.target = {0.0f, mapHeight - viewSize.y / m_camera.zoom};
}

std::optional<Cell> EditorCamera::cellAt(Vector2 screenPos, const Map& map) const {
    const Vector2 world = GetScreenToWorld2D(screenPos, m_camera);
    const auto cellSize = static_cast<float>(kCellSize);
    const Cell cell{static_cast<std::int32_t>(std::floor(world.x / cellSize)),
                    static_cast<std::int32_t>(std::floor(world.y / cellSize))};
    if (!map.inBounds(cell)) {
        return std::nullopt;
    }
    return cell;
}

}  //namespace ljn
