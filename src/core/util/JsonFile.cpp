// JsonFile.cpp - implementation of the text file helpers.

#include "util/JsonFile.hpp"

#include <fstream>
#include <iterator>
#include <system_error>

namespace ljn {

std::expected<std::string, std::string> readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected("cannot open " + path.string());
    }
    return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::expected<void, std::string> writeTextAtomic(const std::filesystem::path& path, std::string_view text) {
    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return std::unexpected("cannot create " + path.parent_path().string() + ": " + error.message());
        }
    }

    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            return std::unexpected("cannot write " + temporary.string());
        }
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file) {
            return std::unexpected("cannot write " + temporary.string());
        }
    }

    std::filesystem::rename(temporary, path, error);
    if (error) {
        return std::unexpected("cannot replace " + path.string() + ": " + error.message());
    }
    return {};
}

}  //namespace ljn
