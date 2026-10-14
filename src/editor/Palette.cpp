// Palette.cpp - implementation of the palette.

#include "Palette.hpp"

#include <filesystem>
#include "map/Animation.hpp"

namespace ljn {
namespace {

//loads an image from the assets folder, with the filter that keeps pixel art sharp
Texture2D loadTexture(const std::string& file) {
    const auto path = std::filesystem::path(LJN_ASSETS_DIR) / file;
    const Texture2D texture = LoadTexture(path.string().c_str());
    if (texture.id != 0) {
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }
    return texture;
}

}  //anonymous namespace

Palette::~Palette() {
    for (const auto& [file, texture] : m_textures) {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
}

void Palette::sync(const Map& map) {
    //loads one image unless it is already loaded
    const auto loadIfMissing = [this](const std::string& file) {
        if (!m_textures.contains(file)) {
            m_textures.emplace(file, loadTexture(file));
        }
    };

    for (const TileDef& def : map.tileDefs()) {
        if (def.isAnimated()) {
            for (const std::string& frame : def.frames) {
                loadIfMissing(frame);
            }
        } else {
            loadIfMissing(def.file);
        }
    }
}

TileId Palette::find(const Map& map, const TileDef& def) {
    const auto defs = map.tileDefs();
    for (std::size_t i = 0; i < defs.size(); ++i) {
        const TileDef& other = defs[i];
        if (other.file == def.file && other.sourceX == def.sourceX && other.sourceY == def.sourceY
            && other.pixelWidth == def.pixelWidth && other.pixelHeight == def.pixelHeight
            && other.solid == def.solid && other.frames == def.frames && other.frameMs == def.frameMs) {
            return static_cast<TileId>(i + 1);
        }
    }
    return kEmptyTile;
}

void Palette::draw(const TileDef& def, Rectangle dest, Color tint) const {
    //an animated tile shows the frame that belongs to the current time
    const auto it = m_textures.find(currentFile(def, m_time));
    if (it == m_textures.end() || it->second.id == 0) {
        DrawRectangleRec(dest, MAGENTA);
        return;
    }

    const Rectangle source{static_cast<float>(def.sourceX), static_cast<float>(def.sourceY),
                           static_cast<float>(def.pixelWidth), static_cast<float>(def.pixelHeight)};
    DrawTexturePro(it->second, source, dest, {0.0f, 0.0f}, 0.0f, tint);
}

}  //namespace ljn
