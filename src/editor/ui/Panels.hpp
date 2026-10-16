// Panels.hpp - the side panel of the editor.

#pragma once

#include <cstdint>
#include <optional>
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

//new settings for one layer, asked for by the panel
struct LayerEdit {
    int index{0};
    LayerInfo info;
};

//what happened in the panel this frame
struct PanelResult {
    bool loadTileClicked{false};
    bool loadAnimatedClicked{false};
    bool openClicked{false};
    bool saveClicked{false};
    bool undoClicked{false};
    bool redoClicked{false};
    bool checkClicked{false};

    //layers tab: add a layer, remove the active one, move it (+1 to the front, -1 to the back)
    bool addLayerClicked{false};
    bool removeLayerClicked{false};
    int moveLayer{0};

    //layers tab: new settings for a layer (name, parallax, solid, visible)
    std::optional<LayerEdit> layerEdit;
};

class SidePanel {
public:
    //draws the panel and handles its buttons, call it outside BeginMode2D
    PanelResult draw(Rectangle area, const Map& map, Palette& palette, ToolState& tools, const PanelInfo& info);

    //true while a text box has the keyboard, so shortcuts must stay quiet
    [[nodiscard]] bool isTyping() const noexcept { return m_nameEditing; }

private:
    enum class Tab : std::uint8_t { Tiles, Layers };

    //the two tabs: what is inside them is drawn from y down to bottom
    void drawTilesTab(float x, float y, float width, float bottom, const Map& map, Palette& palette,
                      PanelResult& result);
    void drawLayersTab(float x, float y, float width, float bottom, const Map& map, ToolState& tools,
                       PanelResult& result);

    Tab m_tab{Tab::Tiles};

    //scroll position of the tile list and of the layer list
    Vector2 m_scroll{0.0f, 0.0f};
    Vector2 m_layerScroll{0.0f, 0.0f};

    //the layer name text box: its text, whether it is being edited and which layer it belongs to
    char m_nameBuffer[33]{};
    bool m_nameEditing{false};
    int m_nameLayer{0};
};

}  //namespace ljn
