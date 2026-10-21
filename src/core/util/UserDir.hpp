// UserDir.hpp - the folder where settings and save data live, one folder for each user of the computer.

#pragma once

#include <filesystem>

namespace ljn {

//linux: $XDG_CONFIG_HOME/log-jam-ninja or ~/.config/log-jam-ninja
//windows: %APPDATA%\log-jam-ninja, mac: ~/Library/Application Support/log-jam-ninja
//if the system does not say, it falls back to a folder named "userdata" next to the executable
[[nodiscard]] std::filesystem::path userDataDir(const std::filesystem::path& executableDir);

}  //namespace ljn
