// Settings.hpp - the options the player can change (screen, sound, keys) and how they are saved.

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include "input/KeyBindings.hpp"

namespace ljn {

struct VideoSettings {
    bool fullscreen{false};

    //only whole multiples of the picture size (sharp pixels), or any size that fits
    bool integerScale{true};
    bool vsync{true};
    bool showFps{false};

    [[nodiscard]] bool operator==(const VideoSettings&) const noexcept = default;
};

//volumes go from 0 to 100
struct AudioSettings {
    int master{80};
    int music{70};
    int sfx{80};

    [[nodiscard]] bool operator==(const AudioSettings&) const noexcept = default;
};

inline constexpr int kMaxVolume = 100;

struct Settings {
    VideoSettings video;
    AudioSettings audio;
    KeyBindings keys;

    [[nodiscard]] bool operator==(const Settings&) const noexcept = default;
};

//the same settings with every number inside its limits
[[nodiscard]] Settings sanitized(Settings settings) noexcept;

//the settings as json text, and back. a field that is missing or has a wrong type keeps its default value
//the only error is text that is not json at all
[[nodiscard]] std::string settingsToJson(const Settings& settings);
[[nodiscard]] std::expected<Settings, std::string> settingsFromJson(const std::string& text);

//the result of loading: the settings to use (defaults when the file is missing or broken) and a message if something was wrong
struct LoadedSettings {
    Settings settings;
    std::string problem;
};

//a file that does not exist is normal (first run), so it gives defaults and no problem
[[nodiscard]] LoadedSettings loadSettings(const std::filesystem::path& path);
[[nodiscard]] std::expected<void, std::string> saveSettings(const std::filesystem::path& path,
                                                            const Settings& settings);

}  //namespace ljn
