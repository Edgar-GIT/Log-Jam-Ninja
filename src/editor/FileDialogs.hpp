// FileDialogs.hpp - the system dialogs for opening and saving map files.

#pragma once

#include <filesystem>
#include <optional>

namespace ljn {

//asks which map file to open, empty if the user cancels
[[nodiscard]] std::optional<std::filesystem::path> askOpenPath();

//asks where to save a map, current is the file the map came from (can be empty), empty result if the user cancels
[[nodiscard]] std::optional<std::filesystem::path> askSavePath(const std::filesystem::path& current);

}  //namespace ljn
