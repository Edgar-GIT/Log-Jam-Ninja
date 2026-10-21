// GamePlay.cpp - the level itself: the player, the camera, the checkpoints and the end of the level.

#include "Game.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>
#include "Style.hpp"
#include "util/TimeFormat.hpp"

namespace ljn {
namespace {

//how fast the free camera moves, in pixels of the picture per second
constexpr float kFreeCameraSpeed = 420.0f;

//how quickly the camera catches up with the player (bigger is faster), and where the player sits vertically
constexpr float kCameraFollow = 8.0f;
constexpr float kPlayerScreenHeight = 0.58f;

constexpr Color kSpawnColor{70, 210, 100, 255};
constexpr Color kCheckpointOff{70, 120, 190, 255};
constexpr Color kCheckpointOn{110, 220, 255, 255};
constexpr Color kPole{235, 235, 240, 255};

constexpr Color kPlayerBody{240, 150, 40, 255};
constexpr Color kPlayerBand{60, 40, 30, 255};
constexpr Color kPlayerEye{255, 255, 255, 255};
constexpr Color kHitbox{80, 255, 120, 255};
constexpr Color kSpriteFrame{255, 220, 80, 255};

//the player sprite is 64x64, the hitbox sits inside it with the feet on the bottom line
constexpr int kSpriteSize = 64;

//one row of the tuning panel: a number of PlayerParams, how much one key press changes it, and its limits
struct Tunable {
    const char* name;
    float PlayerParams::* field;
    float step;
    float min;
    float max;
};

constexpr Tunable kTunables[] = {
    {"hitbox width", &PlayerParams::width, 2.0f, 16.0f, 64.0f},
    {"hitbox height", &PlayerParams::height, 2.0f, 16.0f, 64.0f},
    {"run speed", &PlayerParams::runSpeed, 10.0f, 60.0f, 700.0f},
    {"ground accel", &PlayerParams::groundAccel, 100.0f, 200.0f, 6000.0f},
    {"ground friction", &PlayerParams::groundFriction, 100.0f, 200.0f, 6000.0f},
    {"air accel", &PlayerParams::airAccel, 100.0f, 100.0f, 6000.0f},
    {"air friction", &PlayerParams::airFriction, 100.0f, 0.0f, 6000.0f},
    {"jump height", &PlayerParams::jumpHeight, 4.0f, 16.0f, 300.0f},
    {"tap height", &PlayerParams::tapHeight, 4.0f, 8.0f, 300.0f},
    {"apex time", &PlayerParams::apexTime, 0.01f, 0.10f, 1.00f},
    {"fall gravity x", &PlayerParams::fallGravityScale, 0.05f, 0.5f, 4.0f},
    {"max fall speed", &PlayerParams::maxFall, 25.0f, 200.0f, 1400.0f},
    {"coyote time", &PlayerParams::coyoteTime, 0.01f, 0.0f, 0.4f},
    {"jump buffer", &PlayerParams::jumpBuffer, 0.01f, 0.0f, 0.4f},
};
constexpr int kTunableCount = static_cast<int>(std::size(kTunables));

//a small flag standing on the bottom of a cell, x and y are the top-left corner of the cell on the screen
void drawFlag(int x, int y, Color color) {
    DrawRectangle(x + 6, y + 3, 3, 29, kPole);
    DrawTriangle({static_cast<float>(x + 9), static_cast<float>(y + 3)},
                 {static_cast<float>(x + 9), static_cast<float>(y + 15)},
                 {static_cast<float>(x + 27), static_cast<float>(y + 9)}, color);
}

}  //anonymous namespace

void Game::handlePlayingKeys() {
    //the debug keys: F3 text, F4 hitbox, F2 free camera, R respawn
    if (m_input.keyPressed(KEY_F3)) {
        m_showDebug = !m_showDebug;
    }
    if (m_input.keyPressed(KEY_F4)) {
        m_showHitbox = !m_showHitbox;
    }
    if (m_input.keyPressed(KEY_F2)) {
        m_freeCamera = !m_freeCamera;
    }
    if (m_input.keyPressed(KEY_R) && !m_freeCamera) {
        respawn();
    }
    handleTuningKeys();

    //the pause key opens the pause menu over the level
    if (m_input.pressed(Action::Pause)) {
        buildPauseMenu();
        m_jumpQueued = false;
        m_scenes.push(Scene::Paused);
    }
}

void Game::updatePlaying(float dt) {
    if (!m_level) {
        return;
    }

    //the direction keys, used by the free camera
    m_axis.x = m_input.moveAxis();
    m_axis.y = (m_input.down(Action::Down) ? 1.0f : 0.0f) - (m_input.down(Action::Up) ? 1.0f : 0.0f);

    if (m_freeCamera) {
        m_camera.x += m_axis.x * kFreeCameraSpeed * dt;
        m_camera.y += m_axis.y * kFreeCameraSpeed * dt;
        m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
        return;
    }

    //the player: the movement and the jump come from the bindings
    m_playerInput.move = m_axis.x;
    m_playerInput.jumpHeld = m_input.down(Action::Jump);

    //this step uses the queued jump press, so one press is one jump
    PlayerInput input = m_playerInput;
    input.jumpPressed = m_jumpQueued;
    m_jumpQueued = false;
    step(m_player, m_level->map, input, m_params, dt);
    m_runTime += dt;

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

    //the right edge of the map is the end of the level
    if (reachedGoal(m_player, m_level->map, m_params)) {
        finishLevel();
        return;
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
    if (!m_level) {
        return;
    }
    m_player = spawnState(respawnCell(m_level->map, m_checkpoint), m_params);
    m_jumpQueued = false;

    //the camera jumps to the player, so there is no long slide across the level
    m_camera = {m_player.position.x + m_params.width * 0.5f - static_cast<float>(Screen::kWidth) * 0.5f,
                m_player.position.y + m_params.height * 0.5f - static_cast<float>(Screen::kHeight) * kPlayerScreenHeight};
    m_camera = clampCamera(m_level->map, m_camera, Screen::kWidth, Screen::kHeight);
}

void Game::finishLevel() {
    m_improvement = m_progress.recordClear(m_levelId, {.time = m_runTime, .deaths = m_deaths});
    saveProgress();
    buildCompleteMenu();
    m_scenes.replace(Scene::LevelComplete);
}

void Game::handleTuningKeys() {
    if (m_input.keyPressed(KEY_F5)) {
        m_showTuning = !m_showTuning;
    }
    if (!m_showTuning) {
        return;
    }

    //tab (or shift+tab) selects a row
    const bool shift = m_input.keyDown(KEY_LEFT_SHIFT) || m_input.keyDown(KEY_RIGHT_SHIFT);
    if (m_input.keyPressed(KEY_TAB)) {
        m_tuneIndex = (m_tuneIndex + (shift ? kTunableCount - 1 : 1)) % kTunableCount;
    }

    //page up and page down change the value (hold shift for tiny steps), holding the key repeats
    int direction = 0;
    if (m_input.keyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP)) {
        direction = 1;
    } else if (m_input.keyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN)) {
        direction = -1;
    }
    if (direction != 0) {
        const Tunable& row = kTunables[m_tuneIndex];
        const float oldWidth = m_params.width;
        const float oldHeight = m_params.height;

        float& value = m_params.*row.field;
        value = std::clamp(value + static_cast<float>(direction) * row.step * (shift ? 0.1f : 1.0f), row.min, row.max);

        //a tap can never be higher than the full jump
        m_params.tapHeight = std::min(m_params.tapHeight, m_params.jumpHeight);

        //a new hitbox size keeps the feet and the center where they were
        m_player.position.x += (oldWidth - m_params.width) * 0.5f;
        m_player.position.y += oldHeight - m_params.height;
    }

    //F7 goes back to the default numbers
    if (m_input.keyPressed(KEY_F7)) {
        m_params = PlayerParams{};
        setStatus("Player params reset", false);
    }

    //F6 copies the numbers as C++ code, ready to paste into PlayerParams
    if (m_input.keyPressed(KEY_F6)) {
        std::string text = "PlayerParams{\n";
        text += std::format(
            "    .width = {:.3f}f, .height = {:.3f}f,\n"
            "    .runSpeed = {:.3f}f, .groundAccel = {:.3f}f, .groundFriction = {:.3f}f,\n"
            "    .airAccel = {:.3f}f, .airFriction = {:.3f}f,\n"
            "    .jumpHeight = {:.3f}f, .tapHeight = {:.3f}f, .apexTime = {:.3f}f,\n"
            "    .maxFall = {:.3f}f, .fallGravityScale = {:.3f}f,\n"
            "    .coyoteTime = {:.3f}f, .jumpBuffer = {:.3f}f}};\n",
            m_params.width, m_params.height, m_params.runSpeed, m_params.groundAccel, m_params.groundFriction,
            m_params.airAccel, m_params.airFriction, m_params.jumpHeight, m_params.tapHeight, m_params.apexTime,
            m_params.maxFall, m_params.fallGravityScale, m_params.coyoteTime, m_params.jumpBuffer);
        SetClipboardText(text.c_str());
        setStatus("Player params copied to the clipboard", false);
    }
}

void Game::drawPlaying() {
    if (!m_level) {
        DrawText("No level loaded", 40, Screen::kHeight / 2 - 20, style::kItemSize, style::kText);
        return;
    }

    m_level->tiles.setTime(GetTime());
    const Map& map = m_level->map;

    //layers behind the player, then the markers and the player, then the layers in front of it
    const int split = playerLayer(map);
    drawLayers(map, m_level->tiles, m_camera, Screen::kWidth, Screen::kHeight, 0, split);
    drawMarkers(m_camera);
    drawPlayer(m_camera);
    drawLayers(map, m_level->tiles, m_camera, Screen::kWidth, Screen::kHeight, split + 1, map.layerCount() - 1);

    if (m_showDebug) {
        drawDebug();
    }
    if (m_showTuning) {
        drawTuning();
    }
}

void Game::drawDebug() const {
    DrawRectangle(0, 0, 360, 158, style::kPanel);

    std::string first = std::format("fps {}", GetFPS());
    if (m_settings.video.showFps) {
        first += std::format("  scale {:.2f}x", m_screen.scale());
    }
    if (m_freeCamera) {
        first += "  FREE CAM";
    }
    DrawText(first.c_str(), 8, 6, style::kItemSize, style::kText);

    DrawText(std::format("player {:.0f}, {:.0f}  {}", m_player.position.x, m_player.position.y,
                         m_player.onGround ? "ground" : "air")
                 .c_str(),
             8, 30, style::kItemSize, style::kText);
    DrawText(std::format("speed {:.0f}, {:.0f}", m_player.velocity.x, m_player.velocity.y).c_str(), 8, 54,
             style::kItemSize, style::kText);
    DrawText(std::format("checkpoint {}  deaths {}", m_checkpoint ? *m_checkpoint + 1 : 0, m_deaths).c_str(), 8, 78,
             style::kItemSize, style::kText);
    DrawText(std::format("time {}", formatTime(m_runTime)).c_str(), 8, 102, style::kItemSize, style::kText);
    const int missingTiles = m_level ? m_level->tiles.missingCount() : 0;
    DrawText(std::format("missing sprites {}  tiles {}", m_assets.missing().size(), missingTiles).c_str(), 8, 126,
             style::kItemSize, style::kText);
}

void Game::drawTuning() const {
    //a panel on the right side, one row for each number
    constexpr int kPanelWidth = 340;
    constexpr int kRowHeight = 22;
    const int x = Screen::kWidth - kPanelWidth;
    const int height = 30 + kTunableCount * kRowHeight + 6 * kRowHeight / 2 + 16;
    DrawRectangle(x, 0, kPanelWidth, height, style::kPanel);
    DrawText("PLAYER TUNING", x + 8, 6, style::kItemSize, style::kAccent);

    for (int i = 0; i < kTunableCount; ++i) {
        const Tunable& row = kTunables[i];
        const int y = 30 + i * kRowHeight;
        const bool selected = i == m_tuneIndex;
        if (selected) {
            DrawRectangle(x + 4, y - 1, kPanelWidth - 8, kRowHeight, style::kHighlight);
        }
        const Color color = selected ? style::kAccent : style::kText;
        DrawText(row.name, x + 8, y, style::kItemSize, color);
        DrawText(std::format("{:.2f}", m_params.*row.field).c_str(), x + 210, y, style::kItemSize, color);
    }

    //the numbers the physics works out from the ones above, and the keys
    const int infoY = 30 + kTunableCount * kRowHeight + 6;
    DrawText(std::format("gravity {:.0f}  jump speed {:.0f}", m_params.gravity(), m_params.jumpSpeed()).c_str(), x + 8,
             infoY, style::kItemSize, style::kText);
    DrawText("Tab select   PgUp/PgDn change", x + 8, infoY + 24, style::kSmallSize, style::kTextDim);
    DrawText("Shift: tiny steps   F6 copy code   F7 reset", x + 8, infoY + 38, style::kSmallSize, style::kTextDim);
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

    //the body fills the hitbox, a dark band for the head, and one eye on the side the player looks at
    DrawRectangle(x, y, width, height, kPlayerBody);
    DrawRectangle(x, y + 12, width, 8, kPlayerBand);
    const int eyeX = m_player.facing > 0 ? x + width - 18 : x + 10;
    DrawRectangle(eyeX, y + 24, 8, 8, kPlayerEye);

    if (m_showHitbox) {
        //the 64x64 sprite frame: centered on the hitbox, feet on the same line
        const int spriteX = x - (kSpriteSize - width) / 2;
        const int spriteY = y + height - kSpriteSize;
        DrawRectangleLines(spriteX, spriteY, kSpriteSize, kSpriteSize, kSpriteFrame);
        DrawRectangleLines(x, y, width, height, kHitbox);
    }
}

}  //namespace ljn
