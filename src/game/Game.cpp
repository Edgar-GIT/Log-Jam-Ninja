// Game.cpp - implementation of the game loop: load a map, run and jump through it, checkpoints and respawn.

#include "Game.hpp"

#include <algorithm>
#include <cmath>
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
constexpr float kFreeCameraSpeed = 420.0f;

//how quickly the camera catches up with the player (bigger is faster), and where the player sits vertically
constexpr float kCameraFollow = 8.0f;
constexpr float kPlayerScreenHeight = 0.58f;

constexpr double kStatusSeconds = 5.0;

constexpr Color kSky{70, 110, 160, 255};
constexpr Color kTextColor{235, 235, 240, 255};
constexpr Color kErrorColor{255, 110, 110, 255};
constexpr Color kShade{0, 0, 0, 150};

constexpr Color kPlayerBody{240, 150, 40, 255};
constexpr Color kPlayerBand{60, 40, 30, 255};
constexpr Color kPlayerEye{255, 255, 255, 255};
constexpr Color kHitbox{80, 255, 120, 255};
constexpr Color kSpawnColor{70, 210, 100, 255};
constexpr Color kCheckpointOff{70, 120, 190, 255};
constexpr Color kCheckpointOn{110, 220, 255, 255};
constexpr Color kPole{235, 235, 240, 255};

//a folder that is not found is a real problem, the game cannot work without it
std::filesystem::path assetsOrDie(const std::filesystem::path& executableDir) {
    const auto found = findAssetsDir(executableDir, LJN_ASSETS_DIR);
    if (!found) {
        TraceLog(LOG_ERROR, "ASSETS: no assets folder next to the executable or at %s", LJN_ASSETS_DIR);
        return std::filesystem::path{LJN_ASSETS_DIR};
    }
    return *found;
}

//a small flag standing on the bottom of a cell, x and y are the top-left corner of the cell on the screen
void drawFlag(int x, int y, Color color) {
    DrawRectangle(x + 6, y + 3, 3, 29, kPole);
    DrawTriangle({static_cast<float>(x + 9), static_cast<float>(y + 3)},
                 {static_cast<float>(x + 9), static_cast<float>(y + 15)},
                 {static_cast<float>(x + 27), static_cast<float>(y + 9)}, color);
}

//true if any of the keys is down
bool anyDown(std::initializer_list<int> keys) {
    return std::ranges::any_of(keys, [](int key) { return IsKeyDown(key); });
}

//true if any of the keys went down in this frame
bool anyPressed(std::initializer_list<int> keys) {
    return std::ranges::any_of(keys, [](int key) { return IsKeyPressed(key); });
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
        readInput();

        accumulator += std::min(static_cast<double>(GetFrameTime()), kMaxFrame);
        while (accumulator >= kStep) {
            update(static_cast<float>(kStep));
            accumulator -= kStep;
        }
        draw();
    }
}

void Game::readInput() {
    //window keys: F11 fullscreen, F10 whole-number scale on or off
    if (IsKeyPressed(KEY_F11)) {
        m_screen.toggleFullscreen();
    }
    if (IsKeyPressed(KEY_F10)) {
        m_screen.toggleIntegerScale();
    }

    //debug keys: F3 text, F4 hitbox, F2 free camera, R respawn
    if (IsKeyPressed(KEY_F3)) {
        m_showDebug = !m_showDebug;
    }
    if (IsKeyPressed(KEY_F4)) {
        m_showHitbox = !m_showHitbox;
    }
    if (IsKeyPressed(KEY_F2)) {
        m_freeCamera = !m_freeCamera;
    }
    if (IsKeyPressed(KEY_R) && m_level && !m_freeCamera) {
        respawn();
    }

    //a map file dropped on the window is loaded
    if (IsFileDropped()) {
        const FilePathList dropped = LoadDroppedFiles();
        if (dropped.count > 0) {
            loadLevel(dropped.paths[0]);
        }
        UnloadDroppedFiles(dropped);
    }

    //movement: arrows or A and D, jump: space, up, W, Z or K
    const bool left = anyDown({KEY_LEFT, KEY_A});
    const bool right = anyDown({KEY_RIGHT, KEY_D});
    const bool up = anyDown({KEY_UP, KEY_W});
    const bool down = anyDown({KEY_DOWN, KEY_S});
    m_axis = {static_cast<float>(right) - static_cast<float>(left), static_cast<float>(down) - static_cast<float>(up)};

    m_input.move = m_axis.x;
    m_input.jumpHeld = anyDown({KEY_SPACE, KEY_UP, KEY_W, KEY_Z, KEY_K});

    //the press waits here until a logic step uses it
    if (anyPressed({KEY_SPACE, KEY_UP, KEY_W, KEY_Z, KEY_K})) {
        m_jumpQueued = true;
    }
}

void Game::update(float dt) {
    if (!m_level) {
        return;
    }

    //free camera: no player, just look around
    if (m_freeCamera) {
        m_camera.x += m_axis.x * kFreeCameraSpeed * dt;
        m_camera.y += m_axis.y * kFreeCameraSpeed * dt;
        m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
        return;
    }

    //this step uses the queued jump press, so one press is one jump
    PlayerInput input = m_input;
    input.jumpPressed = m_jumpQueued;
    m_jumpQueued = false;
    step(m_player, m_level->map, input, m_params, dt);

    //touching a checkpoint further ahead makes it the new respawn point (going back does not undo progress)
    const auto touched = checkpointTouched(m_player, m_level->map, m_params);
    if (touched && (!m_checkpoint || *touched > *m_checkpoint)) {
        m_checkpoint = touched;
        setStatus(std::format("Checkpoint {}", *touched + 1), false);
    }

    //falling out of the map is a death
    if (fellOutOfMap(m_player, m_level->map)) {
        ++m_deaths;
        respawn();
    }

    followPlayer(dt);
}

void Game::followPlayer(float dt) {
    //where the camera wants to be: the player in the middle, a bit lower than the center
    const Vector2 target{m_player.position.x + m_params.width * 0.5f - static_cast<float>(Screen::kWidth) * 0.5f,
                         m_player.position.y + m_params.height * 0.5f - static_cast<float>(Screen::kHeight) * kPlayerScreenHeight};

    //exponential smoothing: the same feel at any step size
    const float blend = 1.0f - std::exp(-kCameraFollow * dt);
    m_camera.x += (target.x - m_camera.x) * blend;
    m_camera.y += (target.y - m_camera.y) * blend;
    m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
}

void Game::respawn() {
    m_player = spawnState(respawnCell(m_level->map, m_checkpoint), m_params);
    m_jumpQueued = false;

    //the camera jumps to the player, so there is no long slide across the level
    m_camera = {m_player.position.x + m_params.width * 0.5f - static_cast<float>(Screen::kWidth) * 0.5f,
                m_player.position.y + m_params.height * 0.5f - static_cast<float>(Screen::kHeight) * kPlayerScreenHeight};
    m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
}

void Game::draw() {
    m_screen.beginPicture();
    ClearBackground(kSky);

    if (m_level) {
        m_level->tiles.setTime(GetTime());
        const Map& map = m_level->map;

        //layers behind the player, then the markers and the player, then the layers in front of it
        const int split = playerLayer(map);
        drawLayers(map, m_level->tiles, m_camera, Screen::kWidth, Screen::kHeight, 0, split);
        drawMarkers(m_camera);
        drawPlayer(m_camera);
        drawLayers(map, m_level->tiles, m_camera, Screen::kWidth, Screen::kHeight, split + 1, map.layerCount() - 1);
    } else {
        DrawText("Drop a map file (.json) on the window", 40, Screen::kHeight / 2 - 20, 20, kTextColor);
        DrawText("or start the game with: log-jam-ninja path/to/map.json", 40, Screen::kHeight / 2 + 10, 20,
                 kTextColor);
    }

    //debug text in the corner
    if (m_showDebug) {
        DrawRectangle(0, 0, 360, 134, kShade);
        DrawText(std::format("fps {}  scale {:.2f}x{}", GetFPS(), m_screen.scale(), m_freeCamera ? "  FREE CAM" : "").c_str(),
                 8, 6, 20, kTextColor);
        DrawText(std::format("player {:.0f}, {:.0f}  {}", m_player.position.x, m_player.position.y,
                             m_player.onGround ? "ground" : "air").c_str(), 8, 30, 20, kTextColor);
        DrawText(std::format("speed {:.0f}, {:.0f}", m_player.velocity.x, m_player.velocity.y).c_str(), 8, 54, 20, kTextColor);
        DrawText(std::format("checkpoint {}  deaths {}", m_checkpoint ? *m_checkpoint + 1 : 0, m_deaths).c_str(), 8, 78, 20,
                 kTextColor);
        const int missingTiles = m_level ? m_level->tiles.missingCount() : 0;
        DrawText(std::format("missing sprites {}  tiles {}", m_assets.missing().size(), missingTiles).c_str(), 8, 102, 20,
                 kTextColor);
    }

    //the status message
    if (!m_status.empty() && GetTime() < m_statusUntil) {
        DrawRectangle(0, Screen::kHeight - 34, Screen::kWidth, 34, kShade);
        DrawText(m_status.c_str(), 8, Screen::kHeight - 28, 20, m_statusError ? kErrorColor : kTextColor);
    }

    m_screen.endPicture();
    m_screen.present();
}

void Game::drawMarkers(const Vector2& camera) const {
    //entities use the same whole-pixel camera as the layers with parallax 1
    const auto cameraX = static_cast<int>(std::floor(camera.x));
    const auto cameraY = static_cast<int>(std::floor(camera.y));
    const Map& map = m_level->map;

    if (map.hasSpawn()) {
        drawFlag(map.spawn().x * kCellSize - cameraX, map.spawn().y * kCellSize - cameraY, kSpawnColor);
    }
    const auto checkpoints = map.checkpoints();
    for (std::size_t i = 0; i < checkpoints.size(); ++i) {
        //the checkpoints already reached are bright
        const bool reached = m_checkpoint && static_cast<int>(i) <= *m_checkpoint;
        drawFlag(checkpoints[i].x * kCellSize - cameraX, checkpoints[i].y * kCellSize - cameraY,
                 reached ? kCheckpointOn : kCheckpointOff);
    }
}

void Game::drawPlayer(const Vector2& camera) const {
    const auto x = static_cast<int>(std::floor(m_player.position.x - std::floor(camera.x)));
    const auto y = static_cast<int>(std::floor(m_player.position.y - std::floor(camera.y)));
    const auto width = static_cast<int>(m_params.width);
    const auto height = static_cast<int>(m_params.height);

    //the body, a dark band for the head, and one eye on the side the player looks at
    DrawRectangle(x, y, width, height, kPlayerBody);
    DrawRectangle(x, y + 8, width, 6, kPlayerBand);
    const int eyeX = m_player.facing > 0 ? x + width - 9 : x + 3;
    DrawRectangle(eyeX, y + 16, 6, 6, kPlayerEye);

    if (m_showHitbox) {
        DrawRectangleLines(x, y, width, height, kHitbox);
    }
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

    //a fresh start: no checkpoint, no deaths, the player at the spawn point
    m_checkpoint.reset();
    m_deaths = 0;
    m_freeCamera = false;
    respawn();
    setStatus(std::format("Opened {}", path.filename().string()), false);
}

void Game::setStatus(std::string text, bool error) {
    m_status = std::move(text);
    m_statusError = error;
    m_statusUntil = GetTime() + kStatusSeconds;
}

}  //namespace ljn
