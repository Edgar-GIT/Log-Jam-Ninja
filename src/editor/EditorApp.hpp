// EditorApp.hpp - the map editor program: owns the window, the map and everything that edits it.

#pragma once

#include <nfd.hpp>
#include "Camera.hpp"
#include "Palette.hpp"
#include "Tools.hpp"
#include "map/Map.hpp"
#include "ui/Panels.hpp"
#include "ui/TileImport.hpp"

namespace ljn {

//opens the window when created and closes it when destroyed
class WindowGuard {
public:
    WindowGuard(int width, int height, const char* title);
    ~WindowGuard();

    //there is only one window, so it cannot be copied
    WindowGuard(const WindowGuard&) = delete;
    WindowGuard& operator=(const WindowGuard&) = delete;
};

class EditorApp {
public:
    //opens the window and creates an empty map
    EditorApp();

    //runs the editor until the window is closed
    void run();

private:
    void update(float dt);
    void draw();

    //adds a tile type to the map (or reuses an identical one) and selects it
    void addTile(const TileDef& def);

    //members are destroyed from the last to the first, so the window goes last
    WindowGuard m_window;
    NFD::Guard m_fileDialogs;
    Map m_map;
    EditorCamera m_camera;
    Palette m_palette;
    ToolState m_tools;
    Brush m_brush;
    SidePanel m_panel;
    TileImportDialog m_import;
};

}  //namespace ljn
