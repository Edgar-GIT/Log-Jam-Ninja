// EditorApp.cpp - window setup, main loop and drawing of the map editor.

#include "EditorApp.hpp"

#include <format>
#include <string>
#include <raylib.h>
#include "Grid.hpp"

namespace ljn {
namespace {

constexpr int kStartWidth = 1280;
constexpr int kStartHeight = 720;
constexpr int kDefaultMapWidth = 120;
constexpr int kDefaultMapHeight = 30;

constexpr Color kBackground{18, 18, 24, 255};
constexpr Color kHoverFill{255, 255, 255, 40};
constexpr Color kHoverLine{255, 220, 80, 255};
constexpr Color kTextColor{235, 235, 240, 255};

//current size of the window in pixels
Vector2 viewSize() {
    return {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
}

}  //anonymous namespace

EditorApp::EditorApp() : m_map(Map::create(kDefaultMapWidth, kDefaultMapHeight).value()) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(kStartWidth, kStartHeight, "Log Jam Ninja - Map Editor");
    SetTargetFPS(60);

    //Esc must not close the window, we will use it to cancel things
    SetExitKey(KEY_NULL);

    m_camera.jumpToBottomLeft(viewSize(), m_map);
}

EditorApp::~EditorApp() {
    CloseWindow();
}

void EditorApp::run() {
    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }
}

void EditorApp::update(float dt) {
    m_camera.update(dt, viewSize(), m_map);
}

void EditorApp::draw() const {
    BeginDrawing();
    ClearBackground(kBackground);

    //everything in here is drawn in map coordinates
    BeginMode2D(m_camera.raylib());
    drawGrid(m_map, m_camera.raylib(), viewSize());

    //highlight the cell under the mouse
    const auto hovered = m_camera.cellAt(GetMousePosition(), m_map);
    if (hovered) {
        const auto cell = static_cast<float>(kCellSize);
        const Rectangle rect{static_cast<float>(hovered->x) * cell,
                             static_cast<float>(hovered->y) * cell, cell, cell};
        DrawRectangleRec(rect, kHoverFill);
        DrawRectangleLinesEx(rect, 2.0f / m_camera.zoom(), kHoverLine);
    }
    EndMode2D();

    //text on top, drawn in screen coordinates
    const std::string info = std::format("zoom {:.2f}x   fps {}", m_camera.zoom(), GetFPS());
    DrawText(info.c_str(), 12, 10, 20, kTextColor);
    if (hovered) {
        const std::string cellText = std::format("cell ({}, {})", hovered->x, hovered->y);
        DrawText(cellText.c_str(), 12, 36, 20, kTextColor);
    }
    DrawText("arrows / middle mouse: move    wheel: zoom", 12, GetScreenHeight() - 28, 18, kTextColor);

    EndDrawing();
}

}  //namespace ljn
