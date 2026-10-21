// LevelCatalog.cpp - implementation of the level list.

#include "levels/LevelCatalog.hpp"

#include <algorithm>
#include <cctype>
#include "util/NaturalSort.hpp"

namespace ljn {

std::string prettyLevelName(std::string_view id) {
    std::string name{id};

    //words are separated by spaces, and the first letter is big
    std::ranges::replace(name, '_', ' ');
    std::ranges::replace(name, '-', ' ');
    if (!name.empty()) {
        name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    }
    return name;
}

std::vector<LevelEntry> scanLevels(const std::filesystem::path& folder) {
    std::vector<LevelEntry> levels;

    std::error_code error;
    for (std::filesystem::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error) || it->path().extension() != ".json") {
            continue;
        }
        const std::string id = it->path().stem().string();
        levels.push_back({.id = id, .name = prettyLevelName(id), .path = it->path()});
    }

    //the order of the directory is random, the order of the levels must not be
    std::ranges::sort(levels, [](const LevelEntry& a, const LevelEntry& b) { return naturalLess(a.id, b.id); });
    return levels;
}

bool isUnlocked(std::span<const LevelEntry> levels, const Progress& progress, std::size_t index, bool unlockAll) {
    if (index >= levels.size()) {
        return false;
    }
    return unlockAll || index == 0 || progress.isCompleted(levels[index - 1].id);
}

std::optional<std::size_t> firstUnfinished(std::span<const LevelEntry> levels, const Progress& progress,
                                           bool unlockAll) {
    for (std::size_t i = 0; i < levels.size(); ++i) {
        if (isUnlocked(levels, progress, i, unlockAll) && !progress.isCompleted(levels[i].id)) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> indexOf(std::span<const LevelEntry> levels, std::string_view id) {
    for (std::size_t i = 0; i < levels.size(); ++i) {
        if (levels[i].id == id) {
            return i;
        }
    }
    return std::nullopt;
}

}  //namespace ljn
