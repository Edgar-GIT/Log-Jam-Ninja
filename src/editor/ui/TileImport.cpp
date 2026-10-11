// TileImport.cpp - the add tile window: pick a png, then take the whole image or pieces of a sheet.

#include "ui/TileImport.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <expected>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>
#include <nfd.hpp>
#include <raygui.h>
#include "ui/Text.hpp"

namespace ljn {
namespace {

namespace fs = std::filesystem;

constexpr float kTitleBar = 24.0f;
constexpr float kSideWidth = 280.0f;
constexpr std::array<int, 6> kZoomSteps{1, 2, 3, 4, 6, 8};

constexpr Color kShade{0, 0, 0, 170};
constexpr Color kImageBack{58, 60, 74, 255};
constexpr Color kSliceLine{255, 255, 255, 70};
constexpr Color kSelectionFill{255, 220, 80, 70};
constexpr Color kSelectionLine{255, 220, 80, 255};

//true if both files have exactly the same bytes
bool sameContents(const fs::path& a, const fs::path& b) {
    std::error_code ec;
    const auto sizeA = fs::file_size(a, ec);
    if (ec) {
        return false;
    }
    const auto sizeB = fs::file_size(b, ec);
    if (ec || sizeA != sizeB) {
        return false;
    }
    std::ifstream fileA(a, std::ios::binary);
    std::ifstream fileB(b, std::ios::binary);
    return std::equal(std::istreambuf_iterator<char>(fileA), std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(fileB));
}

//makes sure the image lives inside assets/ and returns its path relative to assets/
std::expected<std::string, std::string> importIntoAssets(const fs::path& source) {
    std::error_code ec;
    const fs::path assets = fs::weakly_canonical(LJN_ASSETS_DIR, ec);
    const fs::path file = fs::weakly_canonical(source, ec);
    if (ec) {
        return std::unexpected("Could not read the path of this file.");
    }

    //already inside assets/: nothing to copy
    const fs::path relative = file.lexically_relative(assets);
    if (!relative.empty() && relative.begin()->string() != "..") {
        return relative.generic_string();
    }

    //otherwise copy it into assets/tiles
    const fs::path folder = assets / "tiles";
    fs::create_directories(folder, ec);
    if (ec) {
        return std::unexpected("Could not create the assets/tiles folder.");
    }

    //never replace a different file that has the same name: add _1, _2... until the name is free
    fs::path target = folder / file.filename();
    for (int copy = 1; fs::exists(target) && !sameContents(file, target); ++copy) {
        target = folder / std::format("{}_{}{}", file.stem().string(), copy, file.extension().string());
    }
    if (!fs::exists(target)) {
        fs::copy_file(file, target, ec);
        if (ec) {
            return std::unexpected("Could not copy the image into assets/tiles.");
        }
    }
    return target.lexically_relative(assets).generic_string();
}

}  //anonymous namespace

TileImportDialog::~TileImportDialog() {
    close();
}

void TileImportDialog::start() {
    //the system file dialog, only png files
    NFD::UniquePath path;
    const nfdfilteritem_t filters[1] = {{"PNG image", "png"}};
    const nfdresult_t result = NFD::OpenDialog(path, filters, 1);

    if (result == NFD_OKAY) {
        open(path.get());
    } else if (result == NFD_ERROR) {
        TraceLog(LOG_WARNING, "File dialog error: %s", NFD::GetError());
    }
}

void TileImportDialog::open(const fs::path& source) {
    close();
    m_open = true;

    //if the image cannot be read, the window opens only to show the problem
    const Texture2D texture = LoadTexture(source.string().c_str());
    if (texture.id == 0) {
        m_message = "Could not read this image.";
        return;
    }
    const auto imported = importIntoAssets(source);
    if (!imported) {
        UnloadTexture(texture);
        m_message = imported.error();
        return;
    }

    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    m_texture = texture;
    m_file = *imported;
}

void TileImportDialog::close() {
    if (m_texture.id != 0) {
        UnloadTexture(m_texture);
    }
    m_texture = Texture2D{};
    m_open = false;
    m_file.clear();
    m_message.clear();
    m_mode = 0;
    m_editW = false;
    m_editH = false;
    m_zoomIndex = kAutoZoom;
    m_scroll = {0.0f, 0.0f};
    clearSelection();
}

void TileImportDialog::clearSelection() {
    m_hasSelection = false;
    m_dragging = false;
}

std::optional<TileDef> TileImportDialog::draw(Vector2 screenSize) {
    if (!m_open) {
        return std::nullopt;
    }

    //dim everything behind the window
    DrawRectangle(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), kShade);

    //the window, centered
    const float width = std::min(screenSize.x - 60.0f, 1040.0f);
    const float height = std::min(screenSize.y - 60.0f, 700.0f);
    const Rectangle window{(screenSize.x - width) * 0.5f, (screenSize.y - height) * 0.5f, width, height};
    if (GuiWindowBox(window, "Add tile") != 0 || IsKeyPressed(KEY_ESCAPE)) {
        close();
        return std::nullopt;
    }

    const float inner = 12.0f;
    const Rectangle body{window.x + inner, window.y + kTitleBar + inner, window.width - 2.0f * inner,
                         window.height - kTitleBar - 2.0f * inner};

    //the image could not be loaded: only show why
    if (m_texture.id == 0) {
        GuiLabel({body.x, body.y, body.width, 30.0f}, m_message.c_str());
        if (GuiButton({body.x, body.y + 40.0f, 140.0f, 40.0f}, "Close")) {
            close();
        }
        return std::nullopt;
    }

    //image on the left, controls on the right
    const Rectangle preview{body.x, body.y, body.width - kSideWidth - inner, body.height};
    const Rectangle side{preview.x + preview.width + inner, body.y, kSideWidth, body.height};
    drawPreview(preview);
    return drawControls(side);
}

void TileImportDialog::drawPreview(Rectangle area) {
    const auto textureWidth = static_cast<float>(m_texture.width);
    const auto textureHeight = static_cast<float>(m_texture.height);

    //first time: pick the biggest zoom where the whole image fits
    if (m_zoomIndex == kAutoZoom) {
        m_zoomIndex = 0;
        for (std::size_t i = 0; i < kZoomSteps.size(); ++i) {
            const auto step = static_cast<float>(kZoomSteps[i]);
            if (textureWidth * step <= area.width - 20.0f && textureHeight * step <= area.height - 20.0f) {
                m_zoomIndex = static_cast<int>(i);
            }
        }
    }
    const auto zoom = static_cast<float>(kZoomSteps[static_cast<std::size_t>(m_zoomIndex)]);

    //a scrollable area that holds the zoomed image
    const Rectangle content{0.0f, 0.0f, textureWidth * zoom, textureHeight * zoom};
    Rectangle view{};
    GuiScrollPanel(area, nullptr, content, &m_scroll, &view);
    const Rectangle image{area.x + m_scroll.x, area.y + m_scroll.y, content.width, content.height};

    //which slice of the sheet is under a screen position (the position is clamped to the image)
    const auto sliceAt = [&](Vector2 point) {
        const int px = std::clamp(static_cast<int>(std::floor((point.x - image.x) / zoom)), 0, m_texture.width - 1);
        const int py = std::clamp(static_cast<int>(std::floor((point.y - image.y) / zoom)), 0, m_texture.height - 1);
        return Slice{px / m_sliceW, py / m_sliceH};
    };

    //sheet mode: click or drag to select one slice or a block of slices
    const Vector2 mouse = GetMousePosition();
    if (m_mode == 1) {
        if (!m_dragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, view)
            && CheckCollisionPointRec(mouse, image)) {
            m_dragging = true;
            m_anchor = sliceAt(mouse);
        }
        if (m_dragging) {
            const Slice current = sliceAt(mouse);
            const int x0 = std::min(m_anchor.x, current.x) * m_sliceW;
            const int y0 = std::min(m_anchor.y, current.y) * m_sliceH;
            const int x1 = std::min((std::max(m_anchor.x, current.x) + 1) * m_sliceW, m_texture.width);
            const int y1 = std::min((std::max(m_anchor.y, current.y) + 1) * m_sliceH, m_texture.height);
            m_selection = {static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1 - x0),
                           static_cast<float>(y1 - y0)};
            m_hasSelection = true;
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                m_dragging = false;
            }
        }
    } else {
        //whole image mode: the selection is always the full image
        m_selection = {0.0f, 0.0f, textureWidth, textureHeight};
        m_hasSelection = true;
    }

    //draw the image, the slice lines and the selection, clipped to the visible part
    BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width),
                     static_cast<int>(view.height));
    DrawRectangleRec(image, kImageBack);
    DrawTexturePro(m_texture, {0.0f, 0.0f, textureWidth, textureHeight}, image, {0.0f, 0.0f}, 0.0f, WHITE);

    if (m_mode == 1) {
        if (static_cast<float>(m_sliceW) * zoom >= 4.0f) {
            for (int x = m_sliceW; x < m_texture.width; x += m_sliceW) {
                DrawRectangleRec({image.x + static_cast<float>(x) * zoom, image.y, 1.0f, image.height}, kSliceLine);
            }
        }
        if (static_cast<float>(m_sliceH) * zoom >= 4.0f) {
            for (int y = m_sliceH; y < m_texture.height; y += m_sliceH) {
                DrawRectangleRec({image.x, image.y + static_cast<float>(y) * zoom, image.width, 1.0f}, kSliceLine);
            }
        }
    }
    if (m_hasSelection) {
        const Rectangle selected{image.x + m_selection.x * zoom, image.y + m_selection.y * zoom,
                                 m_selection.width * zoom, m_selection.height * zoom};
        DrawRectangleRec(selected, kSelectionFill);
        DrawRectangleLinesEx(selected, 2.0f, kSelectionLine);
    }
    EndScissorMode();
}

std::optional<TileDef> TileImportDialog::drawControls(Rectangle area) {
    const float x = area.x;
    const float width = area.width;
    float y = area.y;

    //the image name
    const std::string name = shorten(fs::path(m_file).filename().string(), 22);
    GuiLabel({x, y, width, 26.0f}, name.c_str());
    y += 32.0f;

    //whole image or sheet; changing mode throws away the selection
    const int previousMode = m_mode;
    GuiToggleGroup({x, y, (width - 3.0f) * 0.5f, 36.0f}, "Whole image;Sheet", &m_mode);
    y += 46.0f;
    if (m_mode != previousMode) {
        clearSelection();
    }

    //sheet mode: the size of one slice
    if (m_mode == 1) {
        const int previousW = m_sliceW;
        const int previousH = m_sliceH;

        GuiLabel({x, y, width, 26.0f}, "Slice size (px)");
        y += 30.0f;
        GuiLabel({x, y, 30.0f, 36.0f}, "W");
        if (GuiSpinner({x + 34.0f, y, width - 34.0f, 36.0f}, nullptr, &m_sliceW, 1, m_texture.width, m_editW)) {
            m_editW = !m_editW;
        }
        y += 44.0f;
        GuiLabel({x, y, 30.0f, 36.0f}, "H");
        if (GuiSpinner({x + 34.0f, y, width - 34.0f, 36.0f}, nullptr, &m_sliceH, 1, m_texture.height, m_editH)) {
            m_editH = !m_editH;
        }
        y += 44.0f;

        if (m_sliceW != previousW || m_sliceH != previousH) {
            clearSelection();
        }
        GuiLabel({x, y, width, 24.0f}, "Click or drag on the sheet");
        y += 24.0f;
        GuiLabel({x, y, width, 24.0f}, "to pick one or more slices");
        y += 34.0f;
    }

    //does the tile block movement
    GuiCheckBox({x, y, 28.0f, 28.0f}, "Solid", &m_solid);
    y += 40.0f;

    //zoom of the preview
    const auto stepCount = static_cast<int>(kZoomSteps.size());
    GuiLabel({x, y, 110.0f, 32.0f}, std::format("Zoom {}x", kZoomSteps[static_cast<std::size_t>(m_zoomIndex)]).c_str());
    if (GuiButton({x + 120.0f, y, 60.0f, 32.0f}, "-") && m_zoomIndex > 0) {
        --m_zoomIndex;
        m_scroll = {0.0f, 0.0f};
    }
    if (GuiButton({x + 186.0f, y, 60.0f, 32.0f}, "+") && m_zoomIndex + 1 < stepCount) {
        ++m_zoomIndex;
        m_scroll = {0.0f, 0.0f};
    }
    y += 44.0f;

    //what is selected
    if (m_hasSelection) {
        const int selectedWidth = static_cast<int>(m_selection.width);
        const int selectedHeight = static_cast<int>(m_selection.height);
        GuiLabel({x, y, width, 24.0f}, std::format("{}x{} px at ({}, {})", selectedWidth, selectedHeight,
                                                   static_cast<int>(m_selection.x), static_cast<int>(m_selection.y)).c_str());
        GuiLabel({x, y + 26.0f, width, 24.0f},
                 std::format("{}x{} cells", (selectedWidth + kCellSize - 1) / kCellSize,
                             (selectedHeight + kCellSize - 1) / kCellSize).c_str());
    } else {
        GuiLabel({x, y, width, 24.0f}, "Nothing selected");
    }

    //buttons at the bottom
    const float bottom = area.y + area.height;
    GuiLabel({x, bottom - 134.0f, width, 26.0f}, m_message.c_str());

    if (!m_hasSelection) {
        GuiDisable();
    }
    const bool addPressed = GuiButton({x, bottom - 92.0f, width, 40.0f}, "Add tile");
    GuiEnable();

    if (addPressed && m_hasSelection) {
        const TileDef tile{.file = m_file,
                           .pixelWidth = static_cast<int>(m_selection.width),
                           .pixelHeight = static_cast<int>(m_selection.height),
                           .solid = m_solid,
                           .sourceX = static_cast<int>(m_selection.x),
                           .sourceY = static_cast<int>(m_selection.y)};
        if (m_mode == 0) {
            close();
        } else {
            m_message = std::format("Added a {}x{} tile", tile.pixelWidth, tile.pixelHeight);
        }
        return tile;
    }

    if (GuiButton({x, bottom - 44.0f, width, 40.0f}, "Close")) {
        close();
    }
    return std::nullopt;
}

}  //namespace ljn
