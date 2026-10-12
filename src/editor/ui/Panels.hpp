// Panels.hpp - the side panel of the editor.

#pragma once

#include <raylib.h>
#include "Palette.hpp"
#include "Tools.hpp"
#include "map/Map.hpp"

namespace ljn {

//what the panel needs to know to draw its labels and buttons
struct PanelInfo {
    //result of the last map check, only meaningful when checked is true
    int errors{0};
    int warnings{0};
    bool checked{false};

    bool canUndo{false};
    bool canRedo{false};

    //true when the map has changes that are not saved
    bool dirty{false};
};

//what happened in the panel this frame
struct PanelResult {
    bool loadTileClicked{false};
    bool openClicked{false};
    bool saveClicked{false};
    bool undoClicked{false};
    bool redoClicked{false};
    bool checkClicked{false};
};

class SidePanel {
public:
    //draws the panel and handles its buttons, call it outside BeginMode2D
    PanelResult draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools, const PanelInfo& info);

private:
    //scroll position of the tile list
    Vector2 m_scroll{0.0f, 0.0f};
};

}  //namespace ljn
