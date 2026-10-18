// AssetPath.hpp - finds the assets folder and turns relative asset paths into safe full paths.

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace ljn {

//finds the assets folder: the one next to the executable first (so the game folder can be moved),
//then the one chosen when the project was built, empty if neither exists
[[nodiscard]] std::optional<std::filesystem::path> findAssetsDir(const std::filesystem::path& executableDir,
                                                                 const std::filesystem::path& buildTimeDir);

//root + relative path, or empty if the relative path is absolute, empty or tries to leave the root with ".."
//map files come from outside, so this is what keeps a bad file from reading anything on the disk
[[nodiscard]] std::optional<std::filesystem::path> safeJoin(const std::filesystem::path& root,
                                                            std::string_view relative);

}  //namespace ljn
