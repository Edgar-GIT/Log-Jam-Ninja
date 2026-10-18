// TileSet.cpp - implementation of the tile images of a map.

#include "TileSet.hpp"

#include "assets/AssetPath.hpp"
#include "map/Animation.hpp"

namespace ljn {

TileSet::~TileSet() {
    unloadAll();
}

void TileSet::unloadAll() {
    for (const auto& [file, texture] : m_textures) {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
    m_textures.clear();
    m_missing = 0;
}

void TileSet::load(const Map& map, const std::filesystem::path& root) {
    //the old images go away, a new map has its own tile types
    unloadAll();

    //loads one image unless it is already loaded, a bad or missing path gives an empty texture
    const auto loadIfMissing = [&](const std::string& file) {
        if (m_textures.contains(file)) {
            return;
        }

        Texture2D texture{};
        const auto full = safeJoin(root, file);
        if (full && std::filesystem::exists(*full)) {
            texture = LoadTexture(full->string().c_str());
        }
        if (texture.id != 0) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        } else {
            ++m_missing;
            TraceLog(LOG_WARNING, "TILES: %s not found, using a magenta box", file.c_str());
        }
        m_textures.emplace(file, texture);
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

void TileSet::draw(const TileDef& def, Rectangle dest) const {
    //an animated tile shows the frame that belongs to the current time
    const auto it = m_textures.find(currentFile(def, m_time));
    if (it == m_textures.end() || it->second.id == 0) {
        DrawRectangleRec(dest, MAGENTA);
        return;
    }

    const Rectangle source{static_cast<float>(def.sourceX), static_cast<float>(def.sourceY),
                           static_cast<float>(def.pixelWidth), static_cast<float>(def.pixelHeight)};
    DrawTexturePro(it->second, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

}  //namespace ljn
