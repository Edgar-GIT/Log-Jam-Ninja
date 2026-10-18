// Game.hpp - the game program: window, assets, the loaded level, the player and the main loop.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <raylib.h>
#include "Assets.hpp"
#include "Screen.hpp"
#include "TileSet.hpp"
#include "map/Map.hpp"
#include "sim/Player.hpp"

namespace ljn {

//a map and the images of its tiles
struct Level {
    Level(Map loadedMap, std::filesystem::path path) : map(std::move(loadedMap)), file(std::move(path)) {}

    Map map;
    TileSet tiles;
    std::filesystem::path file;
};

class Game {
public:
    //opens the window and loads the assets, mapPath can be empty (a map can be dropped on the window later)
    Game(const std::filesystem::path& executableDir, const std::filesystem::path& mapPath);

    //runs the game until the window is closed
    void run();

private:
    //keys and dropped files are read once per frame, the logic steps can run zero or many times in a frame
    void readInput();

    //the game logic always advances in steps of 1/60 s, no matter how fast the screen draws
    void update(float dt);
    void draw();

    //loads a map file, puts the player at the spawn point and the camera on the player
    void loadLevel(const std::filesystem::path& path);

    //puts the player at the active checkpoint (or the spawn point) and the camera on it
    void respawn();

    //the camera moves toward the player, smoothly
    void followPlayer(float dt);

    //drawing helpers
    void drawMarkers(const Vector2& camera) const;
    void drawPlayer(const Vector2& camera) const;

    void setStatus(std::string text, bool error);

    //members are destroyed from the last to the first, so the window goes last
    Screen m_screen;
    Assets m_assets;
    std::optional<Level> m_level;

    //the player: rules, state, and the input of the current frame
    PlayerParams m_params;
    PlayerState m_player;
    PlayerInput m_input;

    //a jump press that was not used yet (a frame can end without a logic step, the press must not be lost)
    bool m_jumpQueued{false};

    //the direction keys as an axis, used by the free camera
    Vector2 m_axis{0.0f, 0.0f};

    //index of the last checkpoint reached, and how many times the player died
    std::optional<int> m_checkpoint;
    int m_deaths{0};

    //top-left corner of the view, in world pixels
    Vector2 m_camera{0.0f, 0.0f};

    //F2: fly the camera around instead of playing
    bool m_freeCamera{false};

    //a short message at the bottom
    std::string m_status;
    bool m_statusError{false};
    double m_statusUntil{0.0};

    bool m_showDebug{true};
    bool m_showHitbox{false};
};

}  //namespace ljn
