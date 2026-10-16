// EditorApp.hpp - the map editor program: owns the window, the map and everything that edits it.

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <nfd.hpp>
#include "Camera.hpp"
#include "Palette.hpp"
#include "Tools.hpp"
#include "map/History.hpp"
#include "map/Map.hpp"
#include "ui/AnimImport.hpp"
#include "ui/NewMapDialog.hpp"
#include "ui/Panels.hpp"
#include "ui/TileImport.hpp"
#include "validation/Validator.hpp"

namespace ljn {

//opens the window when created and closes it when destroyed
class WindowGuard {
public:
    WindowGuard(int width, int height, const char* title);
    ~WindowGuard();

    //there is only one window, so it cannot be copied
    WindowGuard(const WindowGuard&) = delete;
    WindowGuard& operator=(const WindowGuard&) = delete;
};

//a short message shown at the bottom of the map for a few seconds
struct Status {
    std::string text;
    bool error{false};
    double until{0.0};
};

//a question that blocks the editor until the user answers
enum class Confirm : std::uint8_t { None, DiscardForOpen, DiscardForNew, DiscardForQuit };

class EditorApp {
public:
    //opens the window and creates an empty map
    EditorApp();

    //runs the editor until the window is closed
    void run();

private:
    void update(float dt);
    void draw();

    //keyboard shortcuts, only called when no window covers the editor
    void handleShortcuts();

    //adds a tile type to the map (or reuses an identical one) and selects it
    void addTile(const TileDef& def);

    //undo and redo, the map check, saving and opening
    void undo();
    void redo();

    //keeps the active layer inside the map, and applies what the layers tab asked for
    void clampLayer();
    void applyLayerActions(const PanelResult& panel);
    void runValidation();
    void checkMap();
    void goToProblem();
    bool save(bool askForPath);
    void requestOpen();
    void openMap();
    void requestNew();
    void createMap(MapSize size);
    void requestQuit();

    //does what the user wanted before the question about unsaved changes
    void continueAfterConfirm(Confirm action);

    //true if the last check still matches the map
    [[nodiscard]] bool reportIsFresh() const noexcept;

    void setStatus(std::string text, bool error);
    void updateTitle();

    //members are destroyed from the last to the first, so the window goes last
    WindowGuard m_window;
    NFD::Guard m_fileDialogs;
    Map m_map;
    History m_history;
    EditorCamera m_camera;
    Palette m_palette;
    ToolState m_tools;
    Brush m_brush;
    SidePanel m_panel;
    TileImportDialog m_import;
    AnimImportDialog m_animImport;
    NewMapDialog m_newMap;

    //the last map check, and the history revision it was made at
    ValidationReport m_report;
    std::optional<unsigned> m_reportRevision;
    std::size_t m_nextProblem{0};

    //the file the map came from or was saved to, empty for a new map
    std::optional<std::filesystem::path> m_path;

    //set when the editor must stop running
    bool m_quit{false};

    Status m_status;
    Confirm m_confirm{Confirm::None};

    //the last window title, so it is only set when it changes
    std::string m_title;
};

}  //namespace ljn
