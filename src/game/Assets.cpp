// Assets.cpp - loads the images listed in the manifest, using reflection to find their paths.

#include "Assets.hpp"

#include <utility>
#include "assets/AssetPath.hpp"
#include "assets/AssetWalk.hpp"

namespace ljn {
namespace {

//size of the square that stands in for a missing image
constexpr int kPlaceholderSize = 64;

//the font is loaded big and drawn smaller, so the letters keep a smooth edge at any size
constexpr int kFontSize = 48;

//the font of the menus
constexpr const char* kFontPath = "fonts/main_font_or_titles.ttf";

//a magenta and black checkerboard: impossible to miss on screen
Texture2D makePlaceholder() {
    const Image image = GenImageChecked(kPlaceholderSize, kPlaceholderSize, 8, 8, MAGENTA, BLACK);
    const Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

}  //anonymous namespace

Assets::Assets(std::filesystem::path root) : m_root(std::move(root)), m_placeholder(makePlaceholder()) {
    //reflection gives the path of every Sprite in the manifest, for example "sprites/player/idle"
    forEachAsset<Sprite>(m_manifest, "", [this](const std::string& name, Sprite& sprite) {
        const std::string relative = name + ".png";
        const auto full = safeJoin(m_root, relative);

        //the file is loaded if it exists, otherwise the placeholder is used and the path is remembered
        if (full && std::filesystem::exists(*full)) {
            const Texture2D texture = LoadTexture(full->string().c_str());
            if (texture.id != 0) {
                SetTextureFilter(texture, TEXTURE_FILTER_POINT);
                sprite = Sprite{texture, false};
                return;
            }
        }
        sprite = Sprite{m_placeholder, true};
        m_missing.push_back(relative);
        TraceLog(LOG_WARNING, "ASSETS: %s not found, using the placeholder", relative.c_str());
    });

    //the menu font, the default font of raylib if the file is missing
    if (const auto full = safeJoin(m_root, kFontPath)) {
        m_menuFont = LoadFontEx(full->string().c_str(), kFontSize, nullptr, 0);
    }
    if (m_menuFont.texture.id == 0) {
        m_menuFont = GetFontDefault();
        TraceLog(LOG_WARNING, "ASSETS: %s not found, using the default font", kFontPath);
    }
}

Assets::~Assets() {
    //only real textures are unloaded here, the placeholder is shared and is unloaded once
    forEachAsset<Sprite>(m_manifest, "", [](const std::string&, Sprite& sprite) {
        if (!sprite.missing) {
            UnloadTexture(sprite.texture);
        }
    });
    UnloadTexture(m_placeholder);

    //the default font belongs to raylib, only a loaded one is unlocked here
    if (m_menuFont.texture.id != 0 && m_menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(m_menuFont);
    }
}

}  //namespace ljn
