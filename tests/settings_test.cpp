// settings_test.cpp - checks the settings: the defaults, the file, the limits and the key bindings.

#include <print>
#include "check.hpp"
#include "settings/Settings.hpp"
#include "util/JsonFile.hpp"

using namespace ljn;

namespace {

//a folder for the temporary files of the test
std::filesystem::path tempFolder() {
    return std::filesystem::temp_directory_path() / "ljn_settings_test";
}

//the default settings have fullscreen off, integer scale on and the volumes in the middle
static void testDefaults() {
    const Settings settings;

    CHECK(!settings.video.fullscreen);
    CHECK(settings.video.integerScale);
    CHECK(settings.video.vsync);
    CHECK(!settings.video.showFps);
    CHECK(settings.audio.master == 80);
    CHECK(settings.audio.music == 70);
    CHECK(settings.audio.sfx == 80);
}

//the numbers are kept inside their limits
static void testSanitized() {
    Settings settings;
    settings.audio.master = 500;
    settings.audio.music = -20;
    settings.audio.sfx = 1000;

    const Settings clean = sanitized(settings);
    CHECK(clean.audio.master == kMaxVolume);
    CHECK(clean.audio.music == 0);
    CHECK(clean.audio.sfx == kMaxVolume);
}

//the settings survive a trip through json
static void testJsonRoundTrip() {
    Settings settings;
    settings.video.fullscreen = true;
    settings.video.integerScale = false;
    settings.video.showFps = true;
    settings.audio.master = 33;
    settings.audio.music = 44;
    settings.audio.sfx = 55;
    CHECK(settings.keys.bind(Action::Jump, 1, key::W));

    const auto loaded = settingsFromJson(settingsToJson(settings));
    CHECK(loaded.has_value());
    CHECK(loaded->video.fullscreen);
    CHECK(!loaded->video.integerScale);
    CHECK(loaded->video.showFps);
    CHECK(loaded->audio.master == 33);
    CHECK(loaded->audio.music == 44);
    CHECK(loaded->audio.sfx == 55);
    CHECK(loaded->keys.key(Action::Jump, 1) == key::W);
}

//text that is not json is an error
static void testBrokenJson() {
    const auto loaded = settingsFromJson("this is not json");
    CHECK(!loaded.has_value());
}

//a field that is missing or has a wrong type keeps the default value
static void testMissingFields() {
    const auto loaded = settingsFromJson("{\"video\": {\"fullscreen\": true, \"vsync\": \"yes\"}}");
    CHECK(loaded.has_value());
    CHECK(loaded->video.fullscreen);
    CHECK(loaded->video.vsync);       //wrong type, the default stays
    CHECK(loaded->video.integerScale); //missing, the default stays
}

//a file that does not exist gives the defaults and no problem
static void testMissingFile() {
    const LoadedSettings loaded = loadSettings(tempFolder() / "does_not_exist.json");
    CHECK(loaded.problem.empty());
    CHECK(loaded.settings == Settings{});
}

//the settings are saved and loaded back
static void testSaveAndLoad() {
    const std::filesystem::path path = tempFolder() / "settings.json";
    std::error_code error;
    std::filesystem::remove_all(tempFolder(), error);

    Settings settings;
    settings.audio.master = 12;
    settings.video.vsync = false;
    CHECK(saveSettings(path, settings).has_value());
    CHECK(std::filesystem::exists(path));

    const LoadedSettings loaded = loadSettings(path);
    CHECK(loaded.problem.empty());
    CHECK(loaded.settings.audio.master == 12);
    CHECK(!loaded.settings.video.vsync);

    std::filesystem::remove_all(tempFolder(), error);
}

//the keys are saved and loaded back too
static void testSavedKeys() {
    const std::filesystem::path path = tempFolder() / "keys.json";
    std::error_code error;
    std::filesystem::remove_all(tempFolder(), error);

    Settings settings;
    CHECK(settings.keys.bind(Action::Attack, 0, key::L));
    CHECK(saveSettings(path, settings).has_value());

    const LoadedSettings loaded = loadSettings(path);
    CHECK(loaded.settings.keys.key(Action::Attack, 0) == key::L);
    //the other key of the same action is the one it had before
    CHECK(loaded.settings.keys.key(Action::Attack, 1) == key::K);

    std::filesystem::remove_all(tempFolder(), error);
}

//a table with a key used twice is refused
static void testBadKeyTable() {
    KeyBindings::Table table{};
    for (std::size_t a = 0; a < kActionCount; ++a) {
        for (int s = 0; s < kSlotsPerAction; ++s) {
            table[a][static_cast<std::size_t>(s)] = key::None;
        }
    }
    table[0][0] = key::A;
    table[1][0] = key::A;
    CHECK(!KeyBindings::fromTable(table).has_value());
}

//a table where an action has no key at all is refused
static void testActionWithoutKeys() {
    KeyBindings::Table table{};
    for (std::size_t a = 0; a < kActionCount; ++a) {
        for (int s = 0; s < kSlotsPerAction; ++s) {
            table[a][static_cast<std::size_t>(s)] = key::None;
        }
    }
    table[0][0] = key::A;
    CHECK(!KeyBindings::fromTable(table).has_value());
}

//a table that breaks the rules gives the default keys back
static void testBadKeysAreIgnored() {
    const auto loaded = settingsFromJson("{\"keys\": {\"left\": [65, 65]}}");
    CHECK(loaded.has_value());
    CHECK(loaded->keys == KeyBindings{});
}

}  //anonymous namespace

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testDefaults();
    testSanitized();
    testJsonRoundTrip();
    testBrokenJson();
    testMissingFields();
    testMissingFile();
    testSaveAndLoad();
    testSavedKeys();
    testBadKeyTable();
    testActionWithoutKeys();
    testBadKeysAreIgnored();
    std::println("all settings tests passed");
}
