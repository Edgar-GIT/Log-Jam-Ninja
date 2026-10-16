// EditorApp.cpp - window setup, main loop, drawing, saving and opening of the map editor.

#include "EditorApp.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <string>
#include <utility>
#include <raygui.h>
#include <raylib.h>
#include "FileDialogs.hpp"
#include "Grid.hpp"
#include "MapRenderer.hpp"
#include "Overlays.hpp"
#include "io/MapSerializer.hpp"

namespace ljn {
namespace {

constexpr int kStartWidth = 1440;
constexpr int kStartHeight = 810;
constexpr int kMinWidth = 1000;
constexpr int kMinHeight = 760;
constexpr int kDefaultMapWidth = 120;
constexpr int kDefaultMapHeight = 30;

//width of the side panel, the map is drawn in the rest of the window
constexpr float kPanelWidth = 300.0f;

//how long a status message stays on screen, in seconds
constexpr double kStatusSeconds = 4.0;

constexpr Color kBackground{18, 18, 24, 255};
constexpr Color kTextColor{235, 235, 240, 255};
constexpr Color kErrorColor{255, 110, 110, 255};
constexpr Color kStatusBack{0, 0, 0, 170};

//size of the area where the map is drawn
Vector2 viewSize() {
    return {static_cast<float>(GetScreenWidth()) - kPanelWidth, static_cast<float>(GetScreenHeight())};
}

//true when the key was just pressed or is repeating because it is held
bool pressedOrRepeated(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

//how many issues of one severity a report has
int countSeverity(const ValidationReport& report, Severity severity) {
    return static_cast<int>(std::ranges::count(report.issues, severity, &Issue::severity));
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
    //a new map starts with three layers: a slow background, the solid ground and a foreground
    static_cast<void>(m_map.addLayer({.name = "Background", .parallaxX = 0.5f, .parallaxY = 0.5f}).value());
    m_tools.layer = m_map.addLayer({.name = "Ground", .solid = true}).value();
    static_cast<void>(m_map.addLayer({.name = "Foreground"}).value());
    m_camera.jumpToBottomLeft(viewSize(), m_map);
}

void EditorApp::run() {
    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }
}

void EditorApp::update(float dt) {
    //animated tiles follow the clock
    m_palette.setTime(GetTime());

    //while a window or a question is open it takes all the input
    const bool inputEnabled = !m_import.isOpen() && !m_animImport.isOpen() && m_confirm == Confirm::None;
    const Vector2 view = viewSize();

    //while a text box has the keyboard, letters and arrows belong to it
    if (inputEnabled && !m_panel.isTyping()) {
        handleShortcuts();

        //a long freeze (for example the file dialog) must not make the camera jump
        m_camera.update(std::min(dt, 0.05f), view, m_map);
    }
    m_brush.update(m_map, m_history, m_palette, m_tools, m_camera, view, inputEnabled);
    updateTitle();
}

void EditorApp::handleShortcuts() {
    const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (ctrl) {
        //Ctrl+S saves, Ctrl+Shift+S asks for a new file, Ctrl+O opens
        if (IsKeyPressed(KEY_S)) {
            save(shift);
        }
        if (IsKeyPressed(KEY_O)) {
            requestOpen();
        }

        //undo and redo are not allowed in the middle of a stroke
        if (!m_brush.busy()) {
            if (pressedOrRepeated(KEY_Z)) {
                if (shift) {
                    redo();
                } else {
                    undo();
                }
            }
            if (pressedOrRepeated(KEY_Y)) {
                redo();
            }
        }
        return;
    }

    //Page Up goes to the layer in front, Page Down to the one behind
    if (IsKeyPressed(KEY_PAGE_UP)) {
        m_tools.layer = std::min(m_tools.layer + 1, m_map.layerCount() - 1);
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        m_tools.layer = std::max(m_tools.layer - 1, 0);
    }

    //tool shortcuts
    if (IsKeyPressed(KEY_B)) {
        m_tools.tool = Tool::Paint;
    }
    if (IsKeyPressed(KEY_E)) {
        m_tools.tool = Tool::Erase;
    }
    if (IsKeyPressed(KEY_S)) {
        m_tools.tool = Tool::Spawn;
    }
    if (IsKeyPressed(KEY_C)) {
        m_tools.tool = Tool::Checkpoint;
    }
    if (IsKeyPressed(KEY_G)) {
        m_tools.snap = !m_tools.snap;
    }
}

void EditorApp::draw() {
    BeginDrawing();
    ClearBackground(kBackground);

    //the map, clipped so it never draws under the side panel
    const Vector2 view = viewSize();
    BeginScissorMode(0, 0, static_cast<int>(view.x), static_cast<int>(view.y));
    BeginMode2D(m_camera.raylib());
    drawGrid(m_map, m_camera.raylib(), view);
    drawTiles(m_map, m_palette, m_camera.raylib(), view, m_tools.layer, m_tools.dimOthers);
    drawMarkers(m_map);
    if (reportIsFresh()) {
        drawIssues(m_report, m_camera.zoom());
    }
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
    if (m_map.hasLayer(m_tools.layer)) {
        const std::string layerText = std::format("layer {}/{}: {}", m_tools.layer + 1, m_map.layerCount(),
                                                  m_map.layerInfo(m_tools.layer).name);
        DrawText(layerText.c_str(), 12, 62, 20, kTextColor);
    }

    //the hints, two lines at the bottom
    DrawText("LMB place   RMB remove   B E S C: tools   G: snap   PgUp / PgDn: layer", 12,
             GetScreenHeight() - 52, 18, kTextColor);
    DrawText("arrows / middle mouse: move   wheel: zoom   Ctrl+S/O: save, open   Ctrl+Z/Y: undo, redo", 12,
             GetScreenHeight() - 28, 18, kTextColor);

    //the status message, it disappears after a few seconds
    if (!m_status.text.empty() && GetTime() < m_status.until) {
        const int textWidth = MeasureText(m_status.text.c_str(), 20);
        const int statusY = GetScreenHeight() - 96;
        DrawRectangle(8, statusY - 6, textWidth + 24, 34, kStatusBack);
        DrawText(m_status.text.c_str(), 20, statusY, 20, m_status.error ? kErrorColor : kTextColor);
    }

    //the side panel, locked while a window or a question is open so it ignores clicks
    const bool modalOpen = m_import.isOpen() || m_animImport.isOpen() || m_confirm != Confirm::None;
    if (modalOpen) {
        GuiLock();
    }
    PanelInfo panelInfo{.canUndo = m_history.canUndo(), .canRedo = m_history.canRedo(), .dirty = m_history.isDirty()};
    if (reportIsFresh()) {
        panelInfo.checked = true;
        panelInfo.errors = countSeverity(m_report, Severity::Error);
        panelInfo.warnings = countSeverity(m_report, Severity::Warning);
    }
    const Rectangle panelArea{view.x, 0.0f, kPanelWidth, view.y};
    const PanelResult panel = m_panel.draw(panelArea, m_map, m_palette, m_tools, panelInfo);
    GuiUnlock();

    //what the user clicked in the panel
    if (panel.loadTileClicked) {
        m_import.start();
    }
    if (panel.loadAnimatedClicked) {
        m_animImport.start();
    }
    if (panel.openClicked) {
        requestOpen();
    }
    if (panel.saveClicked) {
        save(false);
    }
    if (panel.undoClicked) {
        undo();
    }
    if (panel.redoClicked) {
        redo();
    }
    if (panel.checkClicked) {
        checkMap();
    }
    applyLayerActions(panel);

    //the import window goes on top of everything
    const Vector2 screen{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    if (const auto tile = m_import.draw(screen)) {
        addTile(*tile);
    }
    if (const auto tile = m_animImport.draw(screen)) {
        addTile(*tile);
    }

    //the question about unsaved changes: 1 is Save, 2 is Discard, anything else cancels
    if (m_confirm == Confirm::DiscardForOpen) {
        const Rectangle box{screen.x * 0.5f - 220.0f, screen.y * 0.5f - 90.0f, 440.0f, 180.0f};
        const int choice = GuiMessageBox(box, "Unsaved changes", "Save the map before opening another one?",
                                         "Save;Discard;Cancel");
        if (choice >= 0) {
            m_confirm = Confirm::None;
            if (choice == 2 || (choice == 1 && save(false))) {
                openMap();
            }
        }
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

void EditorApp::undo() {
    static_cast<void>(m_history.undo(m_map));
    clampLayer();
}

void EditorApp::redo() {
    static_cast<void>(m_history.redo(m_map));
    clampLayer();
}

void EditorApp::clampLayer() {
    m_tools.layer = std::clamp(m_tools.layer, 0, std::max(0, m_map.layerCount() - 1));
}

void EditorApp::applyLayerActions(const PanelResult& panel) {
    //a stroke must not be cut in half by a layer change
    if (m_brush.busy()) {
        return;
    }

    if (panel.addLayerClicked) {
        LayerInfo info{.name = std::format("Layer {}", m_map.layerCount() + 1)};
        if (const auto index = m_history.addLayer(m_map, std::move(info))) {
            m_tools.layer = *index;
        } else {
            setStatus("Cannot add another layer: the limit was reached", true);
        }
    }
    if (panel.removeLayerClicked && m_history.removeLayer(m_map, m_tools.layer)) {
        clampLayer();
        setStatus("Layer removed (Ctrl+Z brings it back)", false);
    }
    if (panel.moveLayer != 0) {
        const int target = m_tools.layer + panel.moveLayer;
        if (m_history.moveLayer(m_map, m_tools.layer, target)) {
            m_tools.layer = target;
        }
    }
    if (panel.layerEdit) {
        static_cast<void>(m_history.editLayer(m_map, panel.layerEdit->index, panel.layerEdit->info));
    }
}

bool EditorApp::reportIsFresh() const noexcept {
    return m_reportRevision == m_history.revision();
}

void EditorApp::runValidation() {
    //errors first, so "next problem" shows the worst ones before the warnings
    m_report = validate(m_map);
    std::ranges::stable_sort(m_report.issues, std::greater<>{}, &Issue::severity);
    m_reportRevision = m_history.revision();
    m_nextProblem = 0;
}

void EditorApp::checkMap() {
    //the first click checks the map, the next ones go through the problems one by one
    if (!reportIsFresh()) {
        runValidation();
    }
    if (m_report.issues.empty()) {
        setStatus("No problems found", false);
        return;
    }
    goToProblem();
}

void EditorApp::goToProblem() {
    if (m_report.issues.empty()) {
        return;
    }

    //move the view to the problem and say what it is
    m_nextProblem %= m_report.issues.size();
    const Issue& issue = m_report.issues[m_nextProblem];
    m_camera.centerOn(issue.cell, viewSize());
    setStatus(std::format("{} at ({}, {})", describe(issue.code), issue.cell.x, issue.cell.y),
              issue.severity == Severity::Error);
    ++m_nextProblem;
}

bool EditorApp::save(bool askForPath) {
    //the map has to follow the rules before it is written, warnings do not block
    runValidation();
    const int errors = countSeverity(m_report, Severity::Error);
    if (errors > 0) {
        goToProblem();
        setStatus(std::format("Cannot save: {} error(s), see the red cells", errors), true);
        return false;
    }

    //a new map, or "save as", asks where to write
    std::filesystem::path target;
    if (m_path && !askForPath) {
        target = *m_path;
    } else {
        const auto picked = askSavePath(m_path.value_or(std::filesystem::path{}));
        if (!picked) {
            return false;
        }
        target = *picked;
    }

    const auto result = saveToFile(m_map, target);
    if (!result) {
        setStatus(std::format("Save failed: {}", describe(result.error())), true);
        return false;
    }

    m_path = target;
    m_history.markSaved();
    const int warnings = countSeverity(m_report, Severity::Warning);
    setStatus(std::format("Saved {} ({} warnings)", target.filename().string(), warnings), false);
    return true;
}

void EditorApp::requestOpen() {
    //unsaved changes: ask first
    if (m_history.isDirty()) {
        m_confirm = Confirm::DiscardForOpen;
        return;
    }
    openMap();
}

void EditorApp::openMap() {
    const auto path = askOpenPath();
    if (!path) {
        return;
    }

    auto loaded = loadFromFile(*path);
    if (!loaded) {
        setStatus(std::format("Could not open: {}", describe(loaded.error())), true);
        return;
    }

    //the loaded map replaces everything: history, images, check result and view
    m_map = std::move(*loaded);
    m_path = *path;
    m_history.clear();
    m_history.markSaved();
    m_palette.sync(m_map);
    m_palette.select(m_map.tileDefs().empty() ? kEmptyTile : TileId{1});

    //start on the first solid layer, or on the first layer if none is solid
    m_tools.layer = 0;
    for (int layer = 0; layer < m_map.layerCount(); ++layer) {
        if (m_map.layerInfo(layer).solid) {
            m_tools.layer = layer;
            break;
        }
    }
    m_camera.jumpToBottomLeft(viewSize(), m_map);
    m_reportRevision.reset();
    setStatus(std::format("Opened {}", path->filename().string()), false);
}

void EditorApp::setStatus(std::string text, bool error) {
    m_status = Status{std::move(text), error, GetTime() + kStatusSeconds};
}

void EditorApp::updateTitle() {
    //a star after the name means unsaved changes
    const std::string name = m_path ? m_path->filename().string() : "untitled";
    const std::string title = std::format("Log Jam Ninja - Map Editor - {}{}", name, m_history.isDirty() ? "*" : "");
    if (title != m_title) {
        m_title = title;
        SetWindowTitle(m_title.c_str());
    }
}

}  //namespace ljn
