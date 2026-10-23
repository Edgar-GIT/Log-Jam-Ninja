// Assets.hpp - every image the game needs, listed as a struct: the member names are the file paths.

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>
#include <raylib.h>

namespace ljn {

//one image: the texture, and whether it is only a stand in because the file does not exist yet
struct Sprite {
    Texture2D texture{};
    bool missing{true};
};

//the list of images. the path of each one comes from the names: sprites.player.idle is assets/sprites/player/idle.png
//to add an image, add a member here, nothing else is needed
struct PlayerSprites {
    Sprite idle;
    Sprite run;
    Sprite jump;
    Sprite fall;
    Sprite attack;
};

//menu images: the background of each menu and the sheet with all the title buttons (5 rows, 1 idle + 1 hover column)
struct UiSprites {
    Sprite menu_background;
    Sprite menu_buttons;
    Sprite settings_background;
};

struct Sprites {
    PlayerSprites player;
    UiSprites ui;
};

struct Manifest {
    Sprites sprites;
};

//loads every image of the manifest from the assets folder, a missing file becomes a magenta and black square
class Assets {
public:
    //root is the assets folder
    explicit Assets(std::filesystem::path root);
    ~Assets();

    //textures belong to the window, so the assets cannot be copied
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    [[nodiscard]] const std::filesystem::path& root() const noexcept { return m_root; }
    [[nodiscard]] const Manifest& manifest() const noexcept { return m_manifest; }

    //the font of the menus (fonts/main_font_or_titles.ttf), the default font if it is missing
    [[nodiscard]] const Font& menuFont() const noexcept { return m_menuFont; }

    //paths (relative to the root) of the images that were not found
    [[nodiscard]] const std::vector<std::string>& missing() const noexcept { return m_missing; }

private:
    std::filesystem::path m_root;
    Manifest m_manifest;
    std::vector<std::string> m_missing;

    //shared by every missing sprite
    Texture2D m_placeholder{};

    //the font of the menus, loaded at a big size so it can be drawn smaller with a smooth edge
    Font m_menuFont{};
};

}  //namespace ljn
