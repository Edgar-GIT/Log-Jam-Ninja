// UserDir.cpp - implementation of the user data folder.

#include "util/UserDir.hpp"

#include <cstdlib>
#include <string>

namespace ljn {
namespace {

//the value of an environment variable, empty if it does not exist
std::string environment(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string{value} : std::string{};
}

}  //anonymous namespace

std::filesystem::path userDataDir(const std::filesystem::path& executableDir) {
    constexpr const char* kFolder = "log-jam-ninja";

#if defined(_WIN32)
    if (const std::string appData = environment("APPDATA"); !appData.empty()) {
        return std::filesystem::path{appData} / kFolder;
    }
#elif defined(__APPLE__)
    if (const std::string home = environment("HOME"); !home.empty()) {
        return std::filesystem::path{home} / "Library" / "Application Support" / kFolder;
    }
#else
    if (const std::string config = environment("XDG_CONFIG_HOME"); !config.empty()) {
        return std::filesystem::path{config} / kFolder;
    }
    if (const std::string home = environment("HOME"); !home.empty()) {
        return std::filesystem::path{home} / ".config" / kFolder;
    }
#endif
    return executableDir / "userdata";
}

}  //namespace ljn
