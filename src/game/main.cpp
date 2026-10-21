// main.cpp - entry point of the game.

#include <filesystem>
#include <print>
#include <string_view>
#include <raylib.h>
#include "Game.hpp"

//usage: log-jam-ninja [map.json] [--unlock-all]
int main(int argc, char** argv) {
    std::filesystem::path mapPath;
    bool unlockAll = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == "--unlock-all") {
            //every level is open, useful to try them out
            unlockAll = true;
        } else if (argument == "--help" || argument == "-h") {
            std::println("usage: log-jam-ninja [map.json] [--unlock-all]");
            return 0;
        } else {
            mapPath = argument;
        }
    }

    //raylib knows where the executable is (the path ends with a slash)
    ljn::Game game(GetApplicationDirectory(), mapPath, unlockAll);
    game.run();
}
