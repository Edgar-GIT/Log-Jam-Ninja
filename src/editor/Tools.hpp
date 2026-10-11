// Tools.hpp - the editing tools: which one is active, and the brush that paints and erases tiles.

#pragma once

#include <cstdint>
#include <optional>
#include <raylib.h>
#include "Camera.hpp"
#include "Palette.hpp"
#include "map/Map.hpp"

namespace ljn {

enum class Tool : std::uint8_t { Paint, Erase };

//what the user picked in the panel
struct ToolState {
    Tool tool{Tool::Paint};

    //when on, a tile only lands on the grid of its own size (a 2x2 tile only on even cells)
    bool snap{true};
};

class Brush {
public:
    //reads the mouse and edits the map, inputEnabled is false while a window covers the editor
    void update(Map& map, const Palette& palette, const ToolState& tools, const EditorCamera& camera,
                Vector2 viewSize, bool inputEnabled);

    //draws the preview under the mouse, call it inside BeginMode2D
    void drawPreview(const Map& map, const Palette& palette, const ToolState& tools, float zoom) const;

    //cell under the mouse, empty if the mouse is not over the map
    [[nodiscard]] std::optional<Cell> hovered() const noexcept { return m_hovered; }

private:
    enum class Stroke : std::uint8_t { None, Paint, Erase };

    std::optional<Cell> m_hovered;
    Stroke m_stroke{Stroke::None};
};

}  //namespace ljn
