// Palette.hpp - the images of the tile types, and which tile type is selected for painting.

#pragma once

#include <string>
#include <unordered_map>
#include <raylib.h>
#include "map/Map.hpp"

namespace ljn {

class Palette {
public:
    Palette() = default;
    ~Palette();

    //textures belong to the window, so the palette cannot be copied
    Palette(const Palette&) = delete;
    Palette& operator=(const Palette&) = delete;

    //loads the image of every tile type in the map that is not loaded yet
    void sync(const Map& map);

    //id of the tile type that matches this definition exactly, or kEmptyTile if there is none
    [[nodiscard]] static TileId find(const Map& map, const TileDef& def);

    //draws the part of the image that is the tile into a rectangle, a missing image becomes a magenta box
    void draw(const TileDef& def, Rectangle dest, Color tint) const;

    //tile type selected for painting, kEmptyTile if none
    [[nodiscard]] TileId selected() const noexcept { return m_selected; }
    void select(TileId id) noexcept { m_selected = id; }

private:
    //one texture per image file, shared by every tile cut from that file
    std::unordered_map<std::string, Texture2D> m_textures;
    TileId m_selected{kEmptyTile};
};

}  //namespace ljn
