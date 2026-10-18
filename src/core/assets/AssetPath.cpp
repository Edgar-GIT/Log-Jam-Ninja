// AssetPath.cpp - implementation of the assets folder lookup and the safe path join.

#include "assets/AssetPath.hpp"

#include <system_error>

namespace ljn {
namespace {

namespace fs = std::filesystem;

//true if the path is an existing folder
bool isFolder(const fs::path& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

}  //anonymous namespace

std::optional<fs::path> findAssetsDir(const fs::path& executableDir, const fs::path& buildTimeDir) {
    const fs::path next = executableDir / "assets";
    if (isFolder(next)) {
        return next;
    }
    if (!buildTimeDir.empty() && isFolder(buildTimeDir)) {
        return buildTimeDir;
    }
    return std::nullopt;
}

std::optional<fs::path> safeJoin(const fs::path& root, std::string_view relative) {
    if (relative.empty()) {
        return std::nullopt;
    }

    //generic_string paths from the json use '/', and on Windows an absolute path can start with a drive letter
    const fs::path path{std::string(relative)};
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return std::nullopt;
    }
    for (const fs::path& part : path) {
        if (part == "..") {
            return std::nullopt;
        }
    }
    return root / path;
}

}  //namespace ljn
