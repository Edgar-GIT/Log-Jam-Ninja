// JsonFile.hpp - reading and writing small text files (settings, save data) without ever leaving a half written file.

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace ljn {

//the whole content of a file, or a message that says what went wrong
[[nodiscard]] std::expected<std::string, std::string> readTextFile(const std::filesystem::path& path);

//writes the text to path.tmp and then renames it over the real file, so a crash in the middle keeps the old file
//the folders on the way are created if they do not exist
[[nodiscard]] std::expected<void, std::string> writeTextAtomic(const std::filesystem::path& path,
                                                               std::string_view text);

}  //namespace ljn
