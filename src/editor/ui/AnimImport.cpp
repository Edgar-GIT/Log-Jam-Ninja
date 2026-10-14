// AnimImport.cpp - the add animated tile window: pick a folder of png frames, set the speed, add the tile.

#include "ui/AnimImport.hpp"

#include <algorithm>
#include <cmath>
#include <expected>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>
#include <nfd.hpp>
#include <raygui.h>
#include "map/Animation.hpp"
#include "ui/Text.hpp"
#include "util/NaturalSort.hpp"

namespace ljn {
namespace {

namespace fs = std::filesystem;

constexpr float kTitleBar = 24.0f;
constexpr float kSideWidth = 280.0f;
constexpr int kMinIntervalMs = 10;
constexpr int kMaxIntervalMs = 5000;

constexpr Color kShade{0, 0, 0, 170};
constexpr Color kPreviewBack{58, 60, 74, 255};
constexpr Color kPreviewLine{255, 220, 80, 255};

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

//true if the folder already holds a copy of every frame, with the same names and bytes
bool folderHasCopies(const std::vector<fs::path>& files, const fs::path& folder) {
    return std::ranges::all_of(files, [&folder](const fs::path& file) {
        return sameContents(file, folder / file.filename());
    });
}

//makes sure the frames live inside assets/ and returns their paths relative to assets/, in the same order
std::expected<std::vector<std::string>, std::string> importFrames(const std::vector<fs::path>& files) {
    std::error_code ec;
    const fs::path assets = fs::weakly_canonical(LJN_ASSETS_DIR, ec);
    const fs::path source = fs::weakly_canonical(files.front().parent_path(), ec);
    if (ec) {
        return std::unexpected("Could not read the path of this folder.");
    }

    //already inside assets/: nothing to copy
    const fs::path relativeFolder = source.lexically_relative(assets);
    if (!relativeFolder.empty() && relativeFolder.begin()->string() != "..") {
        std::vector<std::string> names;
        for (const fs::path& file : files) {
            names.push_back((relativeFolder / file.filename()).lexically_normal().generic_string());
        }
        return names;
    }

    //otherwise copy them into assets/tiles/<folder name>, never mixing with a different folder of the same name
    const std::string baseName = source.filename().string();
    for (int copy = 0; copy < 1000; ++copy) {
        const std::string name = copy == 0 ? baseName : std::format("{}_{}", baseName, copy);
        const fs::path target = assets / "tiles" / name;
        if (fs::exists(target, ec) && !folderHasCopies(files, target)) {
            continue;
        }

        fs::create_directories(target, ec);
        if (ec) {
            return std::unexpected("Could not create the frames folder in assets/tiles.");
        }
        std::vector<std::string> names;
        for (const fs::path& file : files) {
            const fs::path destination = target / file.filename();
            if (!fs::exists(destination)) {
                fs::copy_file(file, destination, ec);
                if (ec) {
                    return std::unexpected("Could not copy a frame into assets/tiles.");
                }
            }
            names.push_back(destination.lexically_relative(assets).generic_string());
        }
        return names;
    }
    return std::unexpected("Could not find a free folder name in assets/tiles.");
}

}  //anonymous namespace

AnimImportDialog::~AnimImportDialog() {
    close();
}

void AnimImportDialog::start() {
    //the system folder dialog, it starts in the assets folder
    NFD::UniquePath path;
    const std::string assets = fs::path(LJN_ASSETS_DIR).string();
    const nfdresult_t result = NFD::PickFolder(path, assets.c_str());

    if (result == NFD_OKAY) {
        open(path.get());
    } else if (result == NFD_ERROR) {
        TraceLog(LOG_WARNING, "File dialog error: %s", NFD::GetError());
    }
}

void AnimImportDialog::open(const fs::path& folder) {
    close();
    m_open = true;
    m_folder = folder;

    //every png in the folder, in the order a person would expect (frame2 before frame10)
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(folder, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            m_files.push_back(entry.path());
        }
    }
    if (ec) {
        m_message = "Could not read this folder.";
        m_files.clear();
        return;
    }
    std::ranges::sort(m_files, [](const fs::path& a, const fs::path& b) {
        return naturalLess(a.filename().string(), b.filename().string());
    });
    if (m_files.size() < 2) {
        m_message = "The folder needs at least 2 png images.";
        m_files.clear();
        return;
    }

    //load every frame, they all must have the same size
    for (const fs::path& file : m_files) {
        const Image image = LoadImage(file.string().c_str());
        if (image.data == nullptr) {
            m_message = std::format("Could not read {}", file.filename().string());
            unloadFrames();
            return;
        }
        if (m_textures.empty()) {
            m_frameWidth = image.width;
            m_frameHeight = image.height;
        } else if (image.width != m_frameWidth || image.height != m_frameHeight) {
            m_message = std::format("{} is {}x{}, the first frame is {}x{}", file.filename().string(), image.width,
                                    image.height, m_frameWidth, m_frameHeight);
            UnloadImage(image);
            unloadFrames();
            return;
        }
        const Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        m_textures.push_back(texture);
    }

    //the preview asks this tile which frame to show
    m_preview = TileDef{};
    for (const fs::path& file : m_files) {
        m_preview.frames.push_back(file.filename().string());
    }
    m_preview.frameMs = m_intervalMs;
}

void AnimImportDialog::unloadFrames() {
    for (const Texture2D& texture : m_textures) {
        UnloadTexture(texture);
    }
    m_textures.clear();
    m_files.clear();
    m_frameWidth = 0;
    m_frameHeight = 0;
}

void AnimImportDialog::close() {
    unloadFrames();
    m_open = false;
    m_message.clear();
    m_folder.clear();
    m_preview = TileDef{};
    m_editInterval = false;
}

std::optional<TileDef> AnimImportDialog::draw(Vector2 screenSize) {
    if (!m_open) {
        return std::nullopt;
    }

    //dim everything behind the window
    DrawRectangle(0, 0, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), kShade);

    //the window, centered
    const float width = std::min(screenSize.x - 60.0f, 860.0f);
    const float height = std::min(screenSize.y - 60.0f, 560.0f);
    const Rectangle window{(screenSize.x - width) * 0.5f, (screenSize.y - height) * 0.5f, width, height};
    if (GuiWindowBox(window, "Add animated tile") != 0 || IsKeyPressed(KEY_ESCAPE)) {
        close();
        return std::nullopt;
    }

    const float inner = 12.0f;
    const Rectangle body{window.x + inner, window.y + kTitleBar + inner, window.width - 2.0f * inner,
                         window.height - kTitleBar - 2.0f * inner};

    //the frames could not be loaded: only show why
    if (m_textures.empty()) {
        GuiLabel({body.x, body.y, body.width, 30.0f}, m_message.c_str());
        if (GuiButton({body.x, body.y + 40.0f, 140.0f, 40.0f}, "Close")) {
            close();
        }
        return std::nullopt;
    }

    //the animation on the left, controls on the right
    const Rectangle preview{body.x, body.y, body.width - kSideWidth - inner, body.height};
    const Rectangle side{preview.x + preview.width + inner, body.y, kSideWidth, body.height};
    drawPreview(preview);
    return drawControls(side);
}

void AnimImportDialog::drawPreview(Rectangle area) {
    DrawRectangleRec(area, kPreviewBack);

    //the biggest whole zoom that fits in the area
    const auto frameWidth = static_cast<float>(m_frameWidth);
    const auto frameHeight = static_cast<float>(m_frameHeight);
    const float fit = std::floor(std::min((area.width - 20.0f) / frameWidth, (area.height - 20.0f) / frameHeight));
    const float zoom = std::clamp(fit, 1.0f, 16.0f);

    //the frame that belongs to this moment, in the middle of the area
    m_preview.frameMs = m_intervalMs;
    const std::size_t frame = frameAt(m_preview, GetTime());
    const Rectangle dest{area.x + (area.width - frameWidth * zoom) * 0.5f,
                         area.y + (area.height - frameHeight * zoom) * 0.5f, frameWidth * zoom, frameHeight * zoom};
    DrawTexturePro(m_textures[frame], {0.0f, 0.0f, frameWidth, frameHeight}, dest, {0.0f, 0.0f}, 0.0f, WHITE);
    DrawRectangleLinesEx(dest, 1.0f, kPreviewLine);
}

std::optional<TileDef> AnimImportDialog::drawControls(Rectangle area) {
    const float x = area.x;
    const float width = area.width;
    float y = area.y;

    //the folder name and what was found in it
    GuiLabel({x, y, width, 26.0f}, shorten(m_folder.filename().string(), 22).c_str());
    y += 30.0f;
    GuiLabel({x, y, width, 26.0f},
             std::format("{} frames, {}x{} px", m_textures.size(), m_frameWidth, m_frameHeight).c_str());
    y += 40.0f;

    //how long each frame stays on screen
    GuiLabel({x, y, width, 26.0f}, "Time per frame (ms)");
    y += 30.0f;
    if (GuiSpinner({x, y, width, 36.0f}, nullptr, &m_intervalMs, kMinIntervalMs, kMaxIntervalMs, m_editInterval)) {
        m_editInterval = !m_editInterval;
    }
    y += 44.0f;
    GuiLabel({x, y, width, 26.0f}, std::format("{:.1f} frames per second", 1000.0 / m_intervalMs).c_str());
    y += 40.0f;

    //does the tile block movement
    GuiCheckBox({x, y, 28.0f, 28.0f}, "Solid", &m_solid);

    //buttons at the bottom
    const float bottom = area.y + area.height;
    GuiLabel({x, bottom - 134.0f, width, 26.0f}, m_message.c_str());

    if (GuiButton({x, bottom - 92.0f, width, 40.0f}, "Add tile")) {
        const auto frames = importFrames(m_files);
        if (!frames) {
            m_message = frames.error();
            return std::nullopt;
        }
        TileDef tile{.file = frames->front(),
                     .pixelWidth = m_frameWidth,
                     .pixelHeight = m_frameHeight,
                     .solid = m_solid,
                     .frames = *frames,
                     .frameMs = m_intervalMs};
        close();
        return tile;
    }

    if (GuiButton({x, bottom - 44.0f, width, 40.0f}, "Close")) {
        close();
    }
    return std::nullopt;
}

}  //namespace ljn
