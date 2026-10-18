// assets_test.cpp - checks the assets folder lookup, the safe path join and the reflection walk.

#include <filesystem>
#include <print>
#include <string>
#include <system_error>
#include <vector>
#include "assets/AssetPath.hpp"
#include "assets/AssetWalk.hpp"
#include "check.hpp"

using namespace ljn;
namespace fs = std::filesystem;

//the test slot and some groups of slots, like the ones the game will have
struct Slot {
    int value{0};
};
struct PlayerSlots {
    Slot idle;
    Slot run;
};
struct AllSlots {
    PlayerSlots player;
    Slot logo;
    int notASlot{0};
};

//every path the walk finds, in order, and each slot gets its position as value
static void testWalk() {
    AllSlots all;
    std::vector<std::string> paths;
    int counter = 1;
    forEachAsset<Slot>(all, "sprites/", [&](const std::string& path, Slot& slot) {
        paths.push_back(path);
        slot.value = counter++;
    });

    CHECK(paths.size() == 3);
    CHECK(paths[0] == "sprites/player/idle");
    CHECK(paths[1] == "sprites/player/run");
    CHECK(paths[2] == "sprites/logo");

    //the slots were really reached, and other members were left alone
    CHECK(all.player.idle.value == 1 && all.player.run.value == 2 && all.logo.value == 3);
    CHECK(all.notASlot == 0);

    //an empty folder gives paths without a prefix
    std::vector<std::string> plain;
    forEachAsset<Slot>(all, "", [&](const std::string& path, Slot&) { plain.push_back(path); });
    CHECK(plain.front() == "player/idle");

    //a const struct can be walked too
    const AllSlots constant{};
    int count = 0;
    forEachAsset<Slot>(constant, "", [&](const std::string&, const Slot&) { ++count; });
    CHECK(count == 3);
}

//the folder next to the executable wins, then the build folder, otherwise nothing
static void testFindAssetsDir() {
    const fs::path base = fs::temp_directory_path() / "ljn_assets_test";
    std::error_code ec;
    fs::remove_all(base, ec);
    fs::create_directories(base / "exe", ec);
    fs::create_directories(base / "build_assets", ec);

    //only the build folder exists
    const auto fromBuild = findAssetsDir(base / "exe", base / "build_assets");
    CHECK(fromBuild.has_value() && *fromBuild == base / "build_assets");

    //the folder next to the executable appears and wins
    fs::create_directories(base / "exe" / "assets", ec);
    const auto fromExe = findAssetsDir(base / "exe", base / "build_assets");
    CHECK(fromExe.has_value() && *fromExe == base / "exe" / "assets");

    //nothing exists
    CHECK(!findAssetsDir(base / "nowhere", base / "nothing").has_value());
    CHECK(!findAssetsDir(base / "nowhere", {}).has_value());

    fs::remove_all(base, ec);
}

//good relative paths are joined, everything that can escape the folder is refused
static void testSafeJoin() {
    const fs::path root = "/game/assets";
    const auto good = safeJoin(root, "tiles/grass.png");
    CHECK(good.has_value() && *good == root / "tiles/grass.png");

    CHECK(!safeJoin(root, "").has_value());
    CHECK(!safeJoin(root, "/etc/passwd").has_value());
    CHECK(!safeJoin(root, "../secret.png").has_value());
    CHECK(!safeJoin(root, "tiles/../../secret.png").has_value());
    CHECK(!safeJoin(root, "tiles/..").has_value());

    //dots inside a name are fine
    CHECK(safeJoin(root, "tiles/a..b.png").has_value());
}

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testWalk();
    testFindAssetsDir();
    testSafeJoin();
    std::println("all assets tests passed");
}
