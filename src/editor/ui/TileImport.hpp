// TileImport.hpp - the window that adds a tile type: pick a png, then take the whole image or pieces of a sheet.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

class TileImportDialog {
public:
    TileImportDialog() = default;
    ~TileImportDialog();

    //the texture belongs to the window, so the dialog cannot be copied
    TileImportDialog(const TileImportDialog&) = delete;
    TileImportDialog& operator=(const TileImportDialog&) = delete;

    //opens the system file dialog and, if a png is chosen, opens the import window
    void start();

    [[nodiscard]] bool isOpen() const noexcept { return m_open; }

    //draws the window, returns a tile when "Add tile" was pressed
    [[nodiscard]] std::optional<TileDef> draw(Vector2 screenSize);

private:
    //a position on the sheet, counted in slices (not pixels)
    struct Slice {
        int x{0};
        int y{0};
    };

    static constexpr int kAutoZoom = -1;

    void open(const std::filesystem::path& source);
    void close();
    void clearSelection();
    void drawPreview(Rectangle area);
    [[nodiscard]] std::optional<TileDef> drawControls(Rectangle area);

    bool m_open{false};
    Texture2D m_texture{};
    std::string m_file;     //path of the image relative to assets/
    std::string m_message;  //error or info text

    //0 = whole image, 1 = sheet (an int because the raygui toggle group wants one)
    int m_mode{0};

    //size of one slice of the sheet, in pixels
    int m_sliceW{32};
    int m_sliceH{32};
    bool m_editW{false};
    bool m_editH{false};

    bool m_solid{true};

    //which entry of the zoom steps is in use
    int m_zoomIndex{kAutoZoom};
    Vector2 m_scroll{0.0f, 0.0f};

    //the selected part of the image, in pixels
    bool m_dragging{false};
    Slice m_anchor{};
    Rectangle m_selection{};
    bool m_hasSelection{false};
};

}  //namespace ljn
