// main.cpp - entry point of the game.

#include <filesystem>
#include <raylib.h>
#include "Game.hpp"

//usage: log-jam-ninja [map.json]
int main(int argc, char** argv) {
    const std::filesystem::path mapPath = argc > 1 ? std::filesystem::path{argv[1]} : std::filesystem::path{};

    //raylib knows where the executable is (the path ends with a slash)
    ljn::Game game(GetApplicationDirectory(), mapPath);
    game.run();
}
