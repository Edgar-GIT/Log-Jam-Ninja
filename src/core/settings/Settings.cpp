// Settings.cpp - implementation of the settings file.

#include "settings/Settings.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
#include "util/JsonFile.hpp"

namespace ljn {
namespace {

using Json = nlohmann::json;

//the settings file format, in case it has to change one day
constexpr int kVersion = 1;

//copies a boolean from the json only if it is there and really is a boolean
void readBool(const Json& object, const char* name, bool& target) {
    if (const auto found = object.find(name); found != object.end() && found->is_boolean()) {
        target = found->get<bool>();
    }
}

//the same for a whole number
void readInt(const Json& object, const char* name, int& target) {
    if (const auto found = object.find(name); found != object.end() && found->is_number_integer()) {
        target = found->get<int>();
    }
}

//the json object called name, or an empty one
Json section(const Json& root, const char* name) {
    const auto found = root.find(name);
    return found != root.end() && found->is_object() ? *found : Json::object();
}

//the keys of every action, an action that is missing or wrong keeps its default keys
KeyBindings readKeys(const Json& keys) {
    const KeyBindings defaults;
    KeyBindings::Table table{};
    for (std::size_t a = 0; a < kActionCount; ++a) {
        const Action action = kAllActions[a];
        for (int s = 0; s < kSlotsPerAction; ++s) {
            table[a][static_cast<std::size_t>(s)] = defaults.key(action, s);
        }

        const auto found = keys.find(std::string{actionId(action)});
        if (found == keys.end() || !found->is_array()) {
            continue;
        }
        for (std::size_t s = 0; s < static_cast<std::size_t>(kSlotsPerAction); ++s) {
            const bool present = s < found->size() && (*found)[s].is_number_integer();
            table[a][s] = present ? (*found)[s].get<int>() : key::None;
        }
    }

    //a table that breaks the rules (a key used twice, an action without keys) is thrown away completely
    return KeyBindings::fromTable(table).value_or(defaults);
}

}  //anonymous namespace

Settings sanitized(Settings settings) noexcept {
    settings.audio.master = std::clamp(settings.audio.master, 0, kMaxVolume);
    settings.audio.music = std::clamp(settings.audio.music, 0, kMaxVolume);
    settings.audio.sfx = std::clamp(settings.audio.sfx, 0, kMaxVolume);
    return settings;
}

std::string settingsToJson(const Settings& settings) {
    Json keys = Json::object();
    for (const Action action : kAllActions) {
        keys[std::string{actionId(action)}] = Json::array({settings.keys.key(action, 0), settings.keys.key(action, 1)});
    }

    const Json root = {
        {"version", kVersion},
        {"video",
         {{"fullscreen", settings.video.fullscreen},
          {"integerScale", settings.video.integerScale},
          {"vsync", settings.video.vsync},
          {"showFps", settings.video.showFps}}},
        {"audio", {{"master", settings.audio.master}, {"music", settings.audio.music}, {"sfx", settings.audio.sfx}}},
        {"keys", keys},
    };
    return root.dump(2) + "\n";
}

std::expected<Settings, std::string> settingsFromJson(const std::string& text) {
    const Json root = Json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return std::unexpected("the settings file is not valid json");
    }

    Settings settings;
    const Json video = section(root, "video");
    readBool(video, "fullscreen", settings.video.fullscreen);
    readBool(video, "integerScale", settings.video.integerScale);
    readBool(video, "vsync", settings.video.vsync);
    readBool(video, "showFps", settings.video.showFps);

    const Json audio = section(root, "audio");
    readInt(audio, "master", settings.audio.master);
    readInt(audio, "music", settings.audio.music);
    readInt(audio, "sfx", settings.audio.sfx);

    settings.keys = readKeys(section(root, "keys"));
    return sanitized(settings);
}

LoadedSettings loadSettings(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        return {};
    }

    const auto text = readTextFile(path);
    if (!text) {
        return {.settings = {}, .problem = text.error()};
    }
    auto settings = settingsFromJson(*text);
    if (!settings) {
        return {.settings = {}, .problem = settings.error()};
    }
    return {.settings = *settings, .problem = {}};
}

std::expected<void, std::string> saveSettings(const std::filesystem::path& path, const Settings& settings) {
    return writeTextAtomic(path, settingsToJson(sanitized(settings)));
}

}  //namespace ljn
