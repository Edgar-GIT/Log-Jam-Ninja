// LevelCatalog.hpp - the list of levels the game can play: the map files in the levels folder, in natural order.

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "progress/Progress.hpp"

namespace ljn {

struct LevelEntry {
    //the file name without .json, it is the key of the save data
    std::string id;

    //what the menu shows: "level_02" becomes "Level 02"
    std::string name;

    std::filesystem::path path;
};

//"level_02" -> "Level 02", "forest-3" -> "Forest 3"
[[nodiscard]] std::string prettyLevelName(std::string_view id);

//every .json file in the folder, "level2" before "level10". a missing folder gives an empty list
[[nodiscard]] std::vector<LevelEntry> scanLevels(const std::filesystem::path& folder);

//a level is unlocked when it is the first one or the one before it was finished (unlockAll opens everything)
[[nodiscard]] bool isUnlocked(std::span<const LevelEntry> levels, const Progress& progress, std::size_t index,
                              bool unlockAll = false);

//the first unlocked level that was not finished yet, nothing if every level is finished (or there are none)
[[nodiscard]] std::optional<std::size_t> firstUnfinished(std::span<const LevelEntry> levels, const Progress& progress,
                                                         bool unlockAll = false);

//the position of a level in the list
[[nodiscard]] std::optional<std::size_t> indexOf(std::span<const LevelEntry> levels, std::string_view id);

}  //namespace ljn
