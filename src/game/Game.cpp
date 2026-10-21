// Game.cpp - the game loop: the window, the levels, the files of the user and the switch between the scenes.

#include "Game.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>
#include "Style.hpp"
#include "assets/AssetPath.hpp"
#include "io/MapSerializer.hpp"
#include "util/TimeFormat.hpp"
#include "util/UserDir.hpp"

namespace ljn {
namespace {

//one logic step is 1/60 of a second
constexpr double kStep = 1.0 / 60.0;

//the longest frame we accept, after a freeze the game does not try to catch up
constexpr double kMaxFrame = 0.25;

//how long a message stays at the bottom of the screen
constexpr double kStatusSeconds = 5.0;

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

Game::Game(const std::filesystem::path& executableDir, const std::filesystem::path& mapPath, bool unlockAll)
    : m_screen("Log Jam Ninja", false, true, true), m_assets(assetsOrDie(executableDir)), m_unlockAll(unlockAll) {
    //the settings and the save data live in the folder of the user
    const std::filesystem::path userDir = userDataDir(executableDir);
    m_settingsFile = userDir / "settings.json";
    m_progressFile = userDir / "progress.json";

    loadUserFiles();
    applyVideoSettings();

    //the levels of the game are the map files of the assets folder
    m_levelsFolder = m_assets.root() / "maps";
    m_levels = scanLevels(m_levelsFolder);

    buildTitleMenu();

    //a map given in the command line starts right away
    if (!mapPath.empty()) {
        m_levelId = mapPath.stem().string();
        if (loadLevel(mapPath)) {
            m_scenes.reset(Scene::Playing);
        }
    }
}

void Game::run() {
    //the accumulator holds the time that was not simulated yet
    double accumulator = 0.0;

    while (!WindowShouldClose() && !m_quit) {
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
    m_input.update(m_settings.keys);

    //the window keys: F11 fullscreen, F10 whole number scale
    if (m_input.keyPressed(KEY_F11)) {
        m_settings.video.fullscreen = !m_settings.video.fullscreen;
        applyVideoSettings();
    }
    if (m_input.keyPressed(KEY_F10)) {
        m_settings.video.integerScale = !m_settings.video.integerScale;
        applyVideoSettings();
    }

    //a jump press waits for a logic step (a frame can end without one, the press must not be lost)
    if (m_input.pressed(Action::Jump)) {
        m_jumpQueued = true;
    }

    //the keys of the scene that is on top
    switch (m_scenes.current()) {
        case Scene::Title:         handleTitleKeys(); break;
        case Scene::LevelSelect:   handleLevelSelectKeys(); break;
        case Scene::Settings:      handleSettingsKeys(); break;
        case Scene::Controls:      handleControlsKeys(); break;
        case Scene::Playing:       handlePlayingKeys(); break;
        case Scene::Paused:        handlePausedKeys(); break;
        case Scene::LevelComplete: handleLevelCompleteKeys(); break;
    }
}

void Game::update(float dt) {
    //the level is the only scene with logic that runs in steps
    if (m_scenes.current() == Scene::Playing) {
        updatePlaying(dt);
    }
}

void Game::draw() {
    m_screen.beginPicture();
    ClearBackground(style::kBackground);

    switch (m_scenes.current()) {
        case Scene::Title:         drawTitle(); break;
        case Scene::LevelSelect:   drawLevelSelect(); break;
        case Scene::Settings:      drawSettings(); break;
        case Scene::Controls:      drawControls(); break;
        case Scene::Playing:       drawPlaying(); break;
        case Scene::Paused:        drawPaused(); break;
        case Scene::LevelComplete: drawLevelComplete(); break;
    }

    //the status message, at the bottom of everything
    if (!m_status.empty() && GetTime() < m_statusUntil) {
        DrawRectangle(0, Screen::kHeight - 34, Screen::kWidth, 34, style::kPanel);
        DrawText(m_status.c_str(), 8, Screen::kHeight - 28, style::kItemSize,
                 m_statusError ? style::kBad : style::kText);
    }

    m_screen.endPicture();
    m_screen.present();
}

MenuInput Game::menuInput() const {
    return {.up = m_input.menuUp(),
            .down = m_input.menuDown(),
            .left = m_input.menuLeft(),
            .right = m_input.menuRight(),
            .confirm = m_input.menuConfirm(),
            .back = m_input.menuBack()};
}

MenuEvent Game::updateMouse(Menu& menu, const Rectangle& area) {
    //nothing to do if the mouse did not move and no button was pressed
    if (!m_input.mouseMoved() && !m_input.mousePressed()) {
        return {};
    }

    const Vector2 mouse = m_screen.toPicture(m_input.mousePosition());
    const int index = m_menuView.itemAt(menu, mouse, area);
    if (index < 0) {
        return {};
    }

    //the item under the mouse becomes the one under the cursor
    if (m_input.mouseMoved()) {
        menu.hover(index);
    }
    if (m_input.mousePressed()) {
        return menu.click(index, m_menuView.sliderFraction(menu, index, mouse, area));
    }
    return {};
}

void Game::buildTitleMenu() {
    m_titleMenu.clear();
    m_titleMenu.add(Menu::button(kTitlePlay, "Play"));
    m_titleMenu.add(Menu::button(kTitleLevels, "Levels"));
    m_titleMenu.add(Menu::button(kTitleControls, "Controls"));
    m_titleMenu.add(Menu::button(kTitleSettings, "Settings"));
    m_titleMenu.add(Menu::button(kTitleQuit, "Quit"));

    //the title buttons: five in a column in the clearing, below the sign of the background (tune these numbers by eye)
    m_titleButtons = ButtonMenuView{ButtonStack{.centerX = 480.0f, .top = 125.0f, .pitch = 84.0f, .hitWidth = 190.0f},
                                    {{kTitlePlay, 0},
                                     {kTitleLevels, 1},
                                     {kTitleControls, 2},
                                     {kTitleSettings, 3},
                                     {kTitleQuit, 4}},
                                    Rectangle{385.0f, 130.0f, 190.0f, 400.0f}};
    m_titleButtons.setSheet(&m_assets.manifest().sprites.ui.menu_buttons);

    //the sheet is big (1672x941), the buttons are drawn at a bit more than half of their size
    m_titleButtons.setScale(0.45f);
}

void Game::buildLevelMenu() {
    m_levelMenu.clear();
    m_levelMenu.add(Menu::label("CHOOSE A LEVEL"));

    for (std::size_t i = 0; i < m_levels.size(); ++i) {
        const LevelEntry& entry = m_levels[i];
        MenuItem item = Menu::button(kLevelFirst + static_cast<int>(i), entry.name);

        if (!isUnlocked(m_levels, m_progress, i, m_unlockAll)) {
            item.enabled = false;
            item.detail = "locked";
        } else if (const LevelRecord* record = m_progress.find(entry.id); record != nullptr && record->completed) {
            item.detail = formatTime(record->bestTime);
        } else {
            item.detail = "not finished";
        }
        m_levelMenu.add(item);
    }

    m_levelMenu.add(Menu::button(kLevelBack, "Back"));
}

void Game::buildSettingsMenu() {
    m_settingsMenu.clear();
    m_settingsMenu.add(Menu::label("SETTINGS"));
    m_settingsMenu.add(Menu::toggle(kSettingsFullscreen, "Fullscreen", m_settings.video.fullscreen));
    m_settingsMenu.add(Menu::toggle(kSettingsIntegerScale, "Whole number scale", m_settings.video.integerScale));
    m_settingsMenu.add(Menu::toggle(kSettingsVsync, "Limit to 60 fps", m_settings.video.vsync));
    m_settingsMenu.add(Menu::toggle(kSettingsShowFps, "Show the fps", m_settings.video.showFps));
    m_settingsMenu.add(Menu::slider(kSettingsMaster, "Master volume", m_settings.audio.master, 0, kMaxVolume, 5));
    m_settingsMenu.add(Menu::slider(kSettingsMusic, "Music", m_settings.audio.music, 0, kMaxVolume, 5));
    m_settingsMenu.add(Menu::slider(kSettingsSfx, "Effects", m_settings.audio.sfx, 0, kMaxVolume, 5));
    m_settingsMenu.add(Menu::button(kSettingsBack, "Back"));
}

void Game::buildControlsMenu() {
    m_controlsMenu.clear();
    m_controlsMenu.add(Menu::label("CONTROLS"));

    for (const Action action : kAllActions) {
        MenuItem item = Menu::button(kControlAction + static_cast<int>(action), std::string{actionName(action)});
        item.detail = keyName(m_settings.keys.key(action, 0)) + "   " + keyName(m_settings.keys.key(action, 1));
        m_controlsMenu.add(item);
    }

    m_controlsMenu.add(Menu::button(kControlReset, "Default keys"));
    m_controlsMenu.add(Menu::button(kControlBack, "Back"));
}

void Game::buildPauseMenu() {
    m_pauseMenu.clear();
    m_pauseMenu.add(Menu::button(kPauseResume, "Resume"));
    m_pauseMenu.add(Menu::button(kPauseRestart, "Restart level"));
    m_pauseMenu.add(Menu::button(kPauseLevels, "Levels"));
    m_pauseMenu.add(Menu::button(kPauseTitle, "Title"));
}

void Game::buildCompleteMenu() {
    m_completeMenu.clear();
    m_completeMenu.add(Menu::button(kCompleteNext, "Next level"));
    m_completeMenu.add(Menu::button(kCompleteLevels, "Levels"));
    m_completeMenu.add(Menu::button(kCompleteTitle, "Title"));
}

bool Game::loadLevel(const std::filesystem::path& path) {
    auto loaded = loadFromFile(path);
    if (!loaded) {
        setStatus(std::format("Could not open {}: {}", path.filename().string(), describe(loaded.error())), true);
        return false;
    }

    //the new level replaces the old one (the old images are unloaded when it is destroyed)
    m_level.reset();
    m_level.emplace(std::move(*loaded), path);
    m_level->tiles.load(m_level->map, m_assets.root());

    //a fresh start: no checkpoint, no deaths, the clock at zero
    m_checkpoint.reset();
    m_deaths = 0;
    m_runTime = 0.0;
    m_jumpQueued = false;
    m_freeCamera = false;
    respawn();
    return true;
}

void Game::startLevel(std::size_t index) {
    if (index >= m_levels.size()) {
        return;
    }

    const LevelEntry& entry = m_levels[index];
    m_levelId = entry.id;
    m_levelIndex = index;
    if (!loadLevel(entry.path)) {
        return;
    }
    m_scenes.reset(Scene::Playing);
}

void Game::restartLevel() {
    if (!m_level) {
        return;
    }
    const std::filesystem::path path = m_level->file;
    m_checkpoint.reset();
    m_deaths = 0;
    m_runTime = 0.0;
    respawn();
    m_scenes.reset(Scene::Playing);
    setStatus(std::format("Restarted {}", path.filename().string()), false);
}

void Game::loadUserFiles() {
    const LoadedSettings settings = loadSettings(m_settingsFile);
    m_settings = settings.settings;
    if (!settings.problem.empty()) {
        TraceLog(LOG_WARNING, "SETTINGS: %s", settings.problem.c_str());
    }

    const LoadedProgress progress = loadProgress(m_progressFile);
    m_progress = progress.progress;
    if (!progress.problem.empty()) {
        TraceLog(LOG_WARNING, "PROGRESS: %s", progress.problem.c_str());
    }
}

void Game::saveSettings() {
    //the free function of the core, not this one
    if (const auto saved = ljn::saveSettings(m_settingsFile, m_settings); !saved) {
        setStatus(std::format("Could not save the settings: {}", saved.error()), true);
    }
}

void Game::saveProgress() {
    //the free function of the core, not this one
    if (const auto saved = ljn::saveProgress(m_progressFile, m_progress); !saved) {
        setStatus(std::format("Could not save the progress: {}", saved.error()), true);
    }
}

void Game::applyVideoSettings() {
    m_screen.setFullscreen(m_settings.video.fullscreen);
    m_screen.setVsync(m_settings.video.vsync);
    m_screen.setIntegerScale(m_settings.video.integerScale);
}

void Game::setStatus(std::string text, bool error) {
    m_status = std::move(text);
    m_statusError = error;
    m_statusUntil = GetTime() + kStatusSeconds;
}

}  //namespace ljn
