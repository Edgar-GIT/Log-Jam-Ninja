// Panels.hpp - the side panel of the editor.

#pragma once

#include <raylib.h>
#include "Palette.hpp"
#include "Tools.hpp"
#include "map/Map.hpp"

namespace ljn {

//what happened in the panel this frame
struct PanelResult {
    bool loadTileClicked{false};
};

class SidePanel {
public:
    //draws the panel and handles its buttons, call it outside BeginMode2D
    PanelResult draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools);

private:
    //scroll position of the tile list
    Vector2 m_scroll{0.0f, 0.0f};
};

}  //namespace ljn
