// EditorApp.cpp - window setup, main loop and drawing of the map editor.

#include "EditorApp.hpp"

#include <algorithm>
#include <format>
#include <string>
#include <raygui.h>
#include <raylib.h>
#include "Grid.hpp"
#include "MapRenderer.hpp"

namespace ljn {
namespace {

constexpr int kStartWidth = 1440;
constexpr int kStartHeight = 810;
constexpr int kMinWidth = 1000;
constexpr int kMinHeight = 640;
constexpr int kDefaultMapWidth = 120;
constexpr int kDefaultMapHeight = 30;

//width of the side panel, the map is drawn in the rest of the window
constexpr float kPanelWidth = 300.0f;

constexpr Color kBackground{18, 18, 24, 255};
constexpr Color kTextColor{235, 235, 240, 255};

//size of the area where the map is drawn
Vector2 viewSize() {
    return {static_cast<float>(GetScreenWidth()) - kPanelWidth, static_cast<float>(GetScreenHeight())};
}

}  //anonymous namespace

WindowGuard::WindowGuard(int width, int height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title);
    SetWindowMinSize(kMinWidth, kMinHeight);
    SetTargetFPS(60);

    //Esc must not close the window, we use it to cancel things
    SetExitKey(KEY_NULL);

    //bigger text for the ui
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
}

WindowGuard::~WindowGuard() {
    CloseWindow();
}

EditorApp::EditorApp()
    : m_window(kStartWidth, kStartHeight, "Log Jam Ninja - Map Editor"),
      m_map(Map::create(kDefaultMapWidth, kDefaultMapHeight).value()) {
    m_camera.jumpToBottomLeft(viewSize(), m_map);
}

void EditorApp::run() {
    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }
}

void EditorApp::update(float dt) {
    //while the import window is open it takes all the input
    const bool inputEnabled = !m_import.isOpen();
    const Vector2 view = viewSize();

    if (inputEnabled) {
        //keyboard shortcuts
        if (IsKeyPressed(KEY_B)) {
            m_tools.tool = Tool::Paint;
        }
        if (IsKeyPressed(KEY_E)) {
            m_tools.tool = Tool::Erase;
        }
        if (IsKeyPressed(KEY_G)) {
            m_tools.snap = !m_tools.snap;
        }

        //a long freeze (for example the file dialog) must not make the camera jump
        m_camera.update(std::min(dt, 0.05f), view, m_map);
    }
    m_brush.update(m_map, m_palette, m_tools, m_camera, view, inputEnabled);
}

void EditorApp::draw() {
    BeginDrawing();
    ClearBackground(kBackground);

    //the map, clipped so it never draws under the side panel
    const Vector2 view = viewSize();
    BeginScissorMode(0, 0, static_cast<int>(view.x), static_cast<int>(view.y));
    BeginMode2D(m_camera.raylib());
    drawGrid(m_map, m_camera.raylib(), view);
    drawTiles(m_map, m_palette, m_camera.raylib(), view);
    m_brush.drawPreview(m_map, m_palette, m_tools, m_camera.zoom());
    EndMode2D();
    EndScissorMode();

    //text on top of the map
    const std::string info = std::format("zoom {:.2f}x   fps {}", m_camera.zoom(), GetFPS());
    DrawText(info.c_str(), 12, 10, 20, kTextColor);
    if (const auto cell = m_brush.hovered()) {
        const std::string cellText = std::format("cell ({}, {})", cell->x, cell->y);
        DrawText(cellText.c_str(), 12, 36, 20, kTextColor);
    }
    DrawText("arrows / middle mouse: move    wheel: zoom", 12, GetScreenHeight() - 28, 18, kTextColor);

    //the side panel, locked while the import window is open so it ignores clicks
    if (m_import.isOpen()) {
        GuiLock();
    }
    const Rectangle panelArea{view.x, 0.0f, kPanelWidth, view.y};
    const PanelResult panel = m_panel.draw(panelArea, m_map, m_palette, m_tools);
    GuiUnlock();
    if (panel.loadTileClicked) {
        m_import.start();
    }

    //the import window goes on top of everything
    const Vector2 screen{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    if (const auto tile = m_import.draw(screen)) {
        addTile(*tile);
    }

    EndDrawing();
}

void EditorApp::addTile(const TileDef& def) {
    //reuse the tile type if an identical one already exists
    TileId id = Palette::find(m_map, def);
    if (id == kEmptyTile) {
        const auto added = m_map.addTileDef(def);
        if (!added) {
            TraceLog(LOG_WARNING, "Could not add the tile: the palette is full or the tile is too big");
            return;
        }
        id = *added;
        m_palette.sync(m_map);
    }

    //the new tile is ready to paint with
    m_palette.select(id);
    m_tools.tool = Tool::Paint;
}

}  //namespace ljn
