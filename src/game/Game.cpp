// Game.cpp - implementation of the game loop. for now it is a level viewer: load a map, fly the camera around.

#include "Game.hpp"

#include <algorithm>
#include <format>
#include <utility>
#include "LevelRenderer.hpp"
#include "assets/AssetPath.hpp"
#include "io/MapSerializer.hpp"

namespace ljn {
namespace {

//one logic step is 1/60 of a second
constexpr double kStep = 1.0 / 60.0;

//the longest frame we accept, after a freeze (window drag, file dialog) the game does not try to catch up
constexpr double kMaxFrame = 0.25;

//how fast the free camera moves, in pixels of the picture per second
constexpr float kCameraSpeed = 420.0f;

constexpr double kStatusSeconds = 5.0;

constexpr Color kSky{70, 110, 160, 255};
constexpr Color kTextColor{235, 235, 240, 255};
constexpr Color kErrorColor{255, 110, 110, 255};
constexpr Color kShade{0, 0, 0, 150};

//a folder that is not found is a real problem, the game cannot work without it
std::filesystem::path assetsOrDie(const std::filesystem::path& executableDir) {
    const auto found = findAssetsDir(executableDir, LJN_ASSETS_DIR);
    if (!found) {
        TraceLog(LOG_ERROR, "ASSETS: no assets folder next to the executable or at %s", LJN_ASSETS_DIR);
        return std::filesystem::path{LJN_ASSETS_DIR};
    }
    return *found;
}

}  //anonymous namespace

Game::Game(const std::filesystem::path& executableDir, const std::filesystem::path& mapPath)
    : m_screen("Log Jam Ninja"), m_assets(assetsOrDie(executableDir)) {
    if (!mapPath.empty()) {
        loadLevel(mapPath);
    }
}

void Game::run() {
    //the accumulator holds the time that was not simulated yet
    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        accumulator += std::min(static_cast<double>(GetFrameTime()), kMaxFrame);
        while (accumulator >= kStep) {
            update(static_cast<float>(kStep));
            accumulator -= kStep;
        }
        draw();
    }
}

void Game::update(float dt) {
    //window keys: F11 fullscreen, F10 whole-number scale on or off, F3 debug text
    if (IsKeyPressed(KEY_F11)) {
        m_screen.toggleFullscreen();
    }
    if (IsKeyPressed(KEY_F10)) {
        m_screen.toggleIntegerScale();
    }
    if (IsKeyPressed(KEY_F3)) {
        m_showDebug = !m_showDebug;
    }

    //a map file dropped on the window is loaded
    if (IsFileDropped()) {
        const FilePathList dropped = LoadDroppedFiles();
        if (dropped.count > 0) {
            loadLevel(dropped.paths[0]);
        }
        UnloadDroppedFiles(dropped);
    }

    if (!m_level) {
        return;
    }
    m_level->tiles.setTime(GetTime());

    //free camera with the arrows or WASD
    Vector2 move{0.0f, 0.0f};
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        move.x -= 1.0f;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        move.x += 1.0f;
    }
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        move.y -= 1.0f;
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        move.y += 1.0f;
    }
    m_camera.x += move.x * kCameraSpeed * dt;
    m_camera.y += move.y * kCameraSpeed * dt;
    m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
}

void Game::draw() {
    m_screen.beginPicture();
    ClearBackground(kSky);

    if (m_level) {
        drawLevel(m_level->map, m_level->tiles, m_camera, Screen::kWidth, Screen::kHeight);
    } else {
        DrawText("Drop a map file (.json) on the window", 40, Screen::kHeight / 2 - 20, 20, kTextColor);
        DrawText("or start the game with: log-jam-ninja path/to/map.json", 40, Screen::kHeight / 2 + 10, 20,
                 kTextColor);
    }

    //debug text in the corner
    if (m_showDebug) {
        DrawRectangle(0, 0, 330, 108, kShade);
        DrawText(std::format("fps {}  scale {:.2f}x", GetFPS(), m_screen.scale()).c_str(), 8, 6, 20, kTextColor);
        DrawText(std::format("camera {:.0f}, {:.0f}", m_camera.x, m_camera.y).c_str(), 8, 30, 20, kTextColor);
        DrawText(std::format("missing sprites {}", m_assets.missing().size()).c_str(), 8, 54, 20, kTextColor);
        const int missingTiles = m_level ? m_level->tiles.missingCount() : 0;
        DrawText(std::format("missing tiles {}", missingTiles).c_str(), 8, 78, 20, kTextColor);
    }

    //the status message
    if (!m_status.empty() && GetTime() < m_statusUntil) {
        DrawRectangle(0, Screen::kHeight - 34, Screen::kWidth, 34, kShade);
        DrawText(m_status.c_str(), 8, Screen::kHeight - 28, 20, m_statusError ? kErrorColor : kTextColor);
    }

    m_screen.endPicture();
    m_screen.present();
}

void Game::loadLevel(const std::filesystem::path& path) {
    auto loaded = loadFromFile(path);
    if (!loaded) {
        setStatus(std::format("Could not open {}: {}", path.filename().string(), describe(loaded.error())), true);
        return;
    }

    //the new level replaces the old one (the old images are unloaded when it is destroyed)
    m_level.reset();
    m_level.emplace(std::move(*loaded), path);
    m_level->tiles.load(m_level->map, m_assets.root());

    //start at the bottom left, where levels usually begin
    const float bottom = static_cast<float>(m_level->map.height() * kCellSize - Screen::kHeight);
    m_camera = clampCamera(m_level->map, {0.0f, bottom}, Screen::kWidth, Screen::kHeight);
    setStatus(std::format("Opened {}", path.filename().string()), false);
}

void Game::setStatus(std::string text, bool error) {
    m_status = std::move(text);
    m_statusError = error;
    m_statusUntil = GetTime() + kStatusSeconds;
}

}  //namespace ljn
