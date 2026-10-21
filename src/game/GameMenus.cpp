// GameMenus.cpp - the screens of the game that are not the level itself: the title, the level list,
// the settings, the controls, the pause and the end of the level.

#include "Game.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include "Style.hpp"
#include "util/TimeFormat.hpp"

namespace ljn {
namespace {

//where each menu is drawn
constexpr Rectangle kTitleArea{300.0f, 190.0f, 360.0f, 230.0f};
constexpr Rectangle kLevelArea{250.0f, 80.0f, 460.0f, 390.0f};
constexpr Rectangle kSettingsArea{250.0f, 70.0f, 460.0f, 400.0f};
constexpr Rectangle kControlsArea{210.0f, 60.0f, 540.0f, 420.0f};
constexpr Rectangle kPauseArea{330.0f, 160.0f, 300.0f, 190.0f};
constexpr Rectangle kCompleteArea{330.0f, 190.0f, 300.0f, 150.0f};

}  //anonymous namespace

//the title screen: the way into the game
void Game::handleTitleKeys() {
    const MenuEvent event = m_titleMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_titleMenu, kTitleArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    //the title is the first screen, there is nowhere to go back to
    if (chosen.kind != EventKind::Activated) {
        return;
    }

    switch (chosen.id) {
        case kTitlePlay: {
            //the first level that was not finished yet
            if (const auto index = firstUnfinished(m_levels, m_progress, m_unlockAll); index.has_value()) {
                startLevel(*index);
            } else if (!m_levels.empty()) {
                startLevel(0);
            } else {
                setStatus(std::format("No levels in {}", m_levelsFolder.string()), true);
            }
            break;
        }
        case kTitleLevels:
            buildLevelMenu();
            m_scenes.push(Scene::LevelSelect);
            break;
        case kTitleControls:
            buildControlsMenu();
            m_scenes.push(Scene::Controls);
            break;
        case kTitleSettings:
            buildSettingsMenu();
            m_scenes.push(Scene::Settings);
            break;
        case kTitleQuit:
            m_quit = true;
            break;
        default:
            break;
    }
}

void Game::drawTitle() {
    DrawText("LOG JAM NINJA", 240, 70, style::kTitleSize, style::kText);
    DrawText("a ninja made of logs", 340, 130, style::kItemSize, style::kTextDim);
    m_menuView.draw(m_titleMenu, kTitleArea);
    DrawText("arrows or W and S to move   enter to choose   escape to go back", 250, 500, style::kSmallSize,
             style::kTextDim);
}

//the list of the levels, with the record of each one
void Game::handleLevelSelectKeys() {
    const MenuEvent event = m_levelMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_levelMenu, kLevelArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    if (chosen.kind == EventKind::Back || (chosen.kind == EventKind::Activated && chosen.id == kLevelBack)) {
        buildTitleMenu();
        m_scenes.reset(Scene::Title);
        return;
    }
    if (chosen.kind != EventKind::Activated) {
        return;
    }

    const int index = chosen.id - kLevelFirst;
    if (index >= 0 && static_cast<std::size_t>(index) < m_levels.size()) {
        startLevel(static_cast<std::size_t>(index));
    }
}

void Game::drawLevelSelect() {
    DrawText("LEVELS", 430, 34, style::kHeadingSize, style::kText);
    DrawText(std::format("{} of {} finished", m_progress.completedCount(), m_levels.size()).c_str(), 400, 68,
             style::kSmallSize, style::kTextDim);
    m_menuView.draw(m_levelMenu, kLevelArea);
}

//the options of the player
void Game::handleSettingsKeys() {
    const MenuEvent event = m_settingsMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_settingsMenu, kSettingsArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    if (chosen.kind == EventKind::Back || (chosen.kind == EventKind::Activated && chosen.id == kSettingsBack)) {
        saveSettings();
        m_scenes.pop();
        return;
    }
    if (chosen.kind != EventKind::Changed) {
        return;
    }

    //the new value of the item is copied to the settings
    const MenuItem* item = m_settingsMenu.find(chosen.id);
    if (item == nullptr) {
        return;
    }
    switch (chosen.id) {
        case kSettingsFullscreen:    m_settings.video.fullscreen = item->value != 0; break;
        case kSettingsIntegerScale:  m_settings.video.integerScale = item->value != 0; break;
        case kSettingsVsync:         m_settings.video.vsync = item->value != 0; break;
        case kSettingsShowFps:       m_settings.video.showFps = item->value != 0; break;
        case kSettingsMaster:        m_settings.audio.master = item->value; break;
        case kSettingsMusic:         m_settings.audio.music = item->value; break;
        case kSettingsSfx:           m_settings.audio.sfx = item->value; break;
        default: return;
    }
    applyVideoSettings();
}

void Game::drawSettings() {
    DrawText("SETTINGS", 410, 24, style::kHeadingSize, style::kText);
    m_menuView.draw(m_settingsMenu, kSettingsArea);
}

//the keys of the player, which can be changed
void Game::handleControlsKeys() {
    if (m_capturing) {
        //the keys of the frame that opened the capture are ignored
        if (m_captureWait > 0) {
            --m_captureWait;
            return;
        }

        const int pressed = m_input.anyKeyPressed();
        if (pressed == key::None) {
            return;
        }

        m_capturing = false;
        if (pressed == key::Escape) {
            setStatus("Key unchanged", false);
            return;
        }

        const Action action = static_cast<Action>(m_captureAction);
        if (m_settings.keys.bind(action, m_captureSlot, pressed)) {
            setStatus(std::format("{} is now {}", actionName(action), keyName(pressed)), false);
        } else {
            setStatus("That key cannot be used", true);
        }
        buildControlsMenu();
        return;
    }

    const MenuEvent event = m_controlsMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_controlsMenu, kControlsArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    if (chosen.kind == EventKind::Back || (chosen.kind == EventKind::Activated && chosen.id == kControlBack)) {
        m_scenes.pop();
        return;
    }
    if (chosen.kind != EventKind::Activated) {
        return;
    }

    if (chosen.id == kControlReset) {
        m_settings.keys.reset();
        buildControlsMenu();
        setStatus("Default keys", false);
        return;
    }

    //an action: the screen waits for the key that replaces the empty one, or the main one
    const int action = chosen.id - kControlAction;
    if (action < 0 || action >= static_cast<int>(kActionCount)) {
        return;
    }
    const Action selected = static_cast<Action>(action);
    m_captureAction = action;
    m_captureSlot = m_settings.keys.key(selected, 1) == key::None ? 1 : 0;
    m_capturing = true;
    m_captureWait = 8;
}

void Game::drawControls() {
    DrawText("CONTROLS", 405, 20, style::kHeadingSize, style::kText);
    DrawText("choose an action and press enter to give it another key", 260, 54, style::kSmallSize, style::kTextDim);
    m_menuView.draw(m_controlsMenu, kControlsArea);

    if (m_capturing) {
        DrawRectangle(180, 470, 600, 46, style::kPanel);
        DrawText("press a key (escape to keep the old one)", 210, 480, style::kItemSize, style::kAccent);
    }
}

//the pause menu, over the level
void Game::handlePausedKeys() {
    const MenuEvent event = m_pauseMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_pauseMenu, kPauseArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    if (chosen.kind == EventKind::Back || (chosen.kind == EventKind::Activated && chosen.id == kPauseResume)) {
        m_scenes.pop();
        return;
    }
    if (chosen.kind != EventKind::Activated) {
        return;
    }

    switch (chosen.id) {
        case kPauseRestart:
            restartLevel();
            break;
        case kPauseLevels:
            buildLevelMenu();
            m_scenes.reset(Scene::LevelSelect);
            break;
        case kPauseTitle:
            buildTitleMenu();
            m_scenes.reset(Scene::Title);
            break;
        default:
            break;
    }
}

void Game::drawPaused() {
    //the level stays behind the menu, it is drawn first
    drawPlaying();

    DrawRectangle(0, 0, Screen::kWidth, Screen::kHeight, Color{0, 0, 0, 140});
    DrawRectangle(310, 120, 340, 270, style::kPanel);
    DrawText("PAUSED", 430, 132, style::kHeadingSize, style::kText);
    m_menuView.draw(m_pauseMenu, kPauseArea);
}

//the end of the level
void Game::handleLevelCompleteKeys() {
    const MenuEvent event = m_completeMenu.update(menuInput());
    const MenuEvent mouse = updateMouse(m_completeMenu, kCompleteArea);
    const MenuEvent chosen = event.kind != EventKind::None ? event : mouse;

    if (chosen.kind == EventKind::Back || (chosen.kind == EventKind::Activated && chosen.id == kCompleteTitle)) {
        buildTitleMenu();
        m_scenes.reset(Scene::Title);
        return;
    }
    if (chosen.kind != EventKind::Activated) {
        return;
    }

    if (chosen.id == kCompleteNext) {
        //the next level of the list, if it is there and it is open
        if (m_levelIndex.has_value() && *m_levelIndex + 1 < m_levels.size()) {
            const std::size_t next = *m_levelIndex + 1;
            if (isUnlocked(m_levels, m_progress, next, m_unlockAll)) {
                startLevel(next);
                return;
            }
        }
        buildLevelMenu();
        m_scenes.reset(Scene::LevelSelect);
        return;
    }

    if (chosen.id == kCompleteLevels) {
        buildLevelMenu();
        m_scenes.reset(Scene::LevelSelect);
    }
}

void Game::drawLevelComplete() {
    DrawText("LEVEL COMPLETE", 300, 46, style::kHeadingSize, style::kGood);
    DrawText(formatTime(m_runTime).c_str(), 375, 92, style::kHugeSize, style::kText);
    DrawText(std::format("deaths {}", m_deaths).c_str(), 430, 154, style::kItemSize, style::kTextDim);

    if (m_improvement.newBestTime) {
        DrawText("new best time!", 395, 176, style::kItemSize, style::kGood);
    }
    m_menuView.draw(m_completeMenu, kCompleteArea);
}

}  //namespace ljn
