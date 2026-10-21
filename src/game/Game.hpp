// Game.hpp - the game program: window, assets, the menus, the loaded level, the player and the main loop.

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "Assets.hpp"
#include "Input.hpp"
#include "LevelRenderer.hpp"
#include "MenuView.hpp"
#include "Screen.hpp"
#include "TileSet.hpp"
#include "app/SceneStack.hpp"
#include "input/KeyBindings.hpp"
#include "levels/LevelCatalog.hpp"
#include "map/Map.hpp"
#include "progress/Progress.hpp"
#include "settings/Settings.hpp"
#include "sim/Player.hpp"
#include "ui/Menu.hpp"

namespace ljn {

//a map and the images of its tiles
struct Level {
    Level(Map loadedMap, std::filesystem::path path) : map(std::move(loadedMap)), file(std::move(path)) {}

    Map map;
    TileSet tiles;
    std::filesystem::path file;
};

//the ids of the items of the menus, so the code never uses a naked number
enum MenuId : int {
    kTitlePlay = 1,
    kTitleLevels,
    kTitleControls,
    kTitleSettings,
    kTitleQuit,

    kSettingsFullscreen = 10,
    kSettingsIntegerScale,
    kSettingsVsync,
    kSettingsShowFps,
    kSettingsMaster,
    kSettingsMusic,
    kSettingsSfx,
    kSettingsBack,

    //the actions of the controls screen come one after the other, starting here
    kControlAction = 30,
    kControlReset = 50,
    kControlBack,

    //the level list, one id for each level
    kLevelFirst = 100,
    kLevelBack = 900,

    kPauseResume = 1000,
    kPauseRestart,
    kPauseLevels,
    kPauseTitle,

    kCompleteNext = 1010,
    kCompleteLevels,
    kCompleteTitle,
};

class Game {
public:
    //opens the window and loads the assets, mapPath can be empty (the game starts in the title screen)
    Game(const std::filesystem::path& executableDir, const std::filesystem::path& mapPath, bool unlockAll);

    //runs the game until the window is closed
    void run();

private:
    //the loop: the input is read once per frame, the logic runs in steps of 1/60 s
    void readInput();
    void update(float dt);
    void draw();

    //the level, the only scene with logic that runs in steps
    void updatePlaying(float dt);

    //the keys of each scene, read once per frame
    void handleTitleKeys();
    void handleLevelSelectKeys();
    void handleSettingsKeys();
    void handleControlsKeys();
    void handlePlayingKeys();
    void handlePausedKeys();
    void handleLevelCompleteKeys();

    void drawTitle();
    void drawLevelSelect();
    void drawSettings();
    void drawControls();
    void drawPlaying();
    void drawPaused();
    void drawLevelComplete();

    //the menus are built when the scene opens, and again when something changes
    void buildTitleMenu();
    void buildLevelMenu();
    void buildSettingsMenu();
    void buildControlsMenu();
    void buildPauseMenu();
    void buildCompleteMenu();

    //the keys of the menus, in the format the core menu wants
    [[nodiscard]] MenuInput menuInput() const;

    //the mouse over a menu: it selects the item under it and uses it when it is clicked
    [[nodiscard]] MenuEvent updateMouse(Menu& menu, const Rectangle& area);

    //levels
    [[nodiscard]] bool loadLevel(const std::filesystem::path& path);
    void startLevel(std::size_t index);
    void restartLevel();
    void respawn();
    void finishLevel();

    //the files of the user: the settings and the save data
    void loadUserFiles();
    void saveSettings();
    void saveProgress();
    void applyVideoSettings();

    //playing
    void followPlayer(float dt);
    void handleTuningKeys();
    void drawTuning() const;
    void drawDebug() const;
    void drawMarkers(const Vector2& camera) const;
    void drawPlayer(const Vector2& camera) const;

    void setStatus(std::string text, bool error);

    //members are destroyed from the last to the first, so the window goes last
    Screen m_screen;
    Assets m_assets;
    MenuView m_menuView;
    Input m_input;

    //the options of the player and what was done so far
    Settings m_settings;
    Progress m_progress;
    SceneStack m_scenes{Scene::Title};

    //the files of the user, and the levels of the game
    std::filesystem::path m_settingsFile;
    std::filesystem::path m_progressFile;
    std::filesystem::path m_levelsFolder;
    std::vector<LevelEntry> m_levels;
    bool m_unlockAll{false};

    //the level being played
    std::optional<Level> m_level;
    std::optional<std::size_t> m_levelIndex;
    std::string m_levelId;

    //the menus of the scenes
    Menu m_titleMenu;
    Menu m_levelMenu;
    Menu m_settingsMenu;
    Menu m_controlsMenu;
    Menu m_pauseMenu;
    Menu m_completeMenu;

    //the controls screen is waiting for a key
    bool m_capturing{false};
    int m_captureAction{0};
    int m_captureSlot{0};

    //the keys of the frame that opened the capture are ignored for a few steps
    int m_captureWait{0};

    //the player: rules, state, and the input of the current frame
    PlayerParams m_params;
    PlayerState m_player;
    PlayerInput m_playerInput;

    //a jump press that was not used yet (a frame can end without a logic step, the press must not be lost)
    bool m_jumpQueued{false};

    //the run: the checkpoint, the deaths and the time
    std::optional<int> m_checkpoint;
    int m_deaths{0};
    double m_runTime{0.0};
    Improvement m_improvement{};

    //top-left corner of the view, in world pixels
    Vector2 m_camera{0.0f, 0.0f};
    Vector2 m_axis{0.0f, 0.0f};

    //F2: fly the camera around instead of playing
    bool m_freeCamera{false};

    //the game closes the window by itself (the title screen has a quit button)
    bool m_quit{false};

    //a short message at the bottom
    std::string m_status;
    bool m_statusError{false};
    double m_statusUntil{0.0};

    bool m_showDebug{true};
    bool m_showHitbox{false};

    //F5: the tuning panel, and which row of it is selected
    bool m_showTuning{false};
    int m_tuneIndex{0};
};

}  //namespace ljn
