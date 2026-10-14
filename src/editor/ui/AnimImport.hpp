// AnimImport.hpp - the window that adds an animated tile: pick a folder of png frames and set the speed.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

class AnimImportDialog {
public:
    AnimImportDialog() = default;
    ~AnimImportDialog();

    //the textures belong to the window, so the dialog cannot be copied
    AnimImportDialog(const AnimImportDialog&) = delete;
    AnimImportDialog& operator=(const AnimImportDialog&) = delete;

    //opens the system folder dialog and, if a folder is chosen, opens the import window
    void start();

    [[nodiscard]] bool isOpen() const noexcept { return m_open; }

    //draws the window, returns a tile when "Add tile" was pressed
    [[nodiscard]] std::optional<TileDef> draw(Vector2 screenSize);

private:
    void open(const std::filesystem::path& folder);
    void close();
    void unloadFrames();
    void drawPreview(Rectangle area);
    [[nodiscard]] std::optional<TileDef> drawControls(Rectangle area);

    bool m_open{false};
    std::string m_message;  //error or info text
    std::filesystem::path m_folder;

    //the frames in play order, and one texture for each, only for the preview
    std::vector<std::filesystem::path> m_files;
    std::vector<Texture2D> m_textures;
    int m_frameWidth{0};
    int m_frameHeight{0};

    //a tile with the same frame count and speed, only used to ask which frame to show
    TileDef m_preview;

    int m_intervalMs{150};
    bool m_editInterval{false};
    bool m_solid{true};
};

}  //namespace ljn
