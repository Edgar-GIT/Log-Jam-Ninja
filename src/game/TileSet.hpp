// TileSet.hpp - the images of the tile types of one map, loaded from the assets folder.

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

class TileSet {
public:
    TileSet() = default;
    ~TileSet();

    //textures belong to the window, so the tile set cannot be copied
    TileSet(const TileSet&) = delete;
    TileSet& operator=(const TileSet&) = delete;

    //loads the image of every tile type of the map (every frame of animated tiles), root is the assets folder
    void load(const Map& map, const std::filesystem::path& root);

    //how many image files could not be loaded
    [[nodiscard]] int missingCount() const noexcept { return m_missing; }

    //the time used to pick the frame of animated tiles
    void setTime(double seconds) noexcept { m_time = seconds; }

    //draws the part of the image that is the tile, a missing image becomes a magenta box
    void draw(const TileDef& def, Rectangle dest) const;

private:
    void unloadAll();

    //one texture per image file, shared by every tile cut from that file
    std::unordered_map<std::string, Texture2D> m_textures;
    double m_time{0.0};
    int m_missing{0};
};

}  //namespace ljn
