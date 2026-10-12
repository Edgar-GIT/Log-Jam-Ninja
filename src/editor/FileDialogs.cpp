// FileDialogs.cpp - implementation of the map file dialogs.

#include "FileDialogs.hpp"

#include <string>
#include <system_error>
#include <nfd.hpp>
#include <raylib.h>

namespace ljn {
namespace {

//the folder where maps are kept, assets/maps, created if it does not exist
std::filesystem::path mapsFolder() {
    const auto folder = std::filesystem::path(LJN_ASSETS_DIR) / "maps";
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    return folder;
}

}  //anonymous namespace

std::optional<std::filesystem::path> askOpenPath() {
    //only json files, starting in the maps folder
    NFD::UniquePath path;
    const nfdfilteritem_t filters[1] = {{"Map file", "json"}};
    const std::string folder = mapsFolder().string();
    const nfdresult_t result = NFD::OpenDialog(path, filters, 1, folder.c_str());

    if (result == NFD_OKAY) {
        return std::filesystem::path(path.get());
    }
    if (result == NFD_ERROR) {
        TraceLog(LOG_WARNING, "File dialog error: %s", NFD::GetError());
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> askSavePath(const std::filesystem::path& current) {
    //start in the folder of the current file, or in the maps folder for a new map
    const std::string folder = (current.empty() ? mapsFolder() : current.parent_path()).string();
    const std::string name = current.empty() ? "level.json" : current.filename().string();

    NFD::UniquePath path;
    const nfdfilteritem_t filters[1] = {{"Map file", "json"}};
    const nfdresult_t result = NFD::SaveDialog(path, filters, 1, folder.c_str(), name.c_str());

    if (result == NFD_OKAY) {
        //the dialog does not add the extension by itself
        std::filesystem::path chosen(path.get());
        if (chosen.extension() != ".json") {
            chosen += ".json";
        }
        return chosen;
    }
    if (result == NFD_ERROR) {
        TraceLog(LOG_WARNING, "File dialog error: %s", NFD::GetError());
    }
    return std::nullopt;
}

}  //namespace ljn
