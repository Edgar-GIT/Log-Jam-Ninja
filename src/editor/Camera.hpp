// Camera.hpp - the editor view: moving, zooming and converting the mouse to a grid cell.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

class EditorCamera {
public:
    //reads the input, moves and zooms the view, then keeps it inside the map
    void update(float dt, Vector2 viewSize, const Map& map);

    //puts the view on the bottom left corner of the map
    void jumpToBottomLeft(Vector2 viewSize, const Map& map);

    //centers the view on a cell, the next update keeps it inside the map
    void centerOn(Cell cell, Vector2 viewSize);

    //the raylib camera, to use with BeginMode2D
    [[nodiscard]] const Camera2D& raylib() const noexcept { return m_camera; }

    //zoom factor, 1 means one map pixel per screen pixel
    [[nodiscard]] float zoom() const noexcept { return m_camera.zoom; }

    //grid cell under a screen position, or nothing if the position is outside the map
    [[nodiscard]] std::optional<Cell> cellAt(Vector2 screenPos, const Map& map) const;

private:
    //the zoom steps the mouse wheel goes through
    static constexpr std::array<float, 6> kZoomLevels{0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f};

    //offset is always 0, so "target" is the map position of the top left corner of the window
    Camera2D m_camera{.offset = {0.0f, 0.0f}, .target = {0.0f, 0.0f}, .rotation = 0.0f, .zoom = 1.0f};

    //which entry of kZoomLevels is in use (starts at 1.0)
    std::size_t m_zoomIndex = 2;
};

}  //namespace ljn
