// Player.hpp - the player's movement rules: run, jump, gravity and collision with the solid tiles of a map.

#pragma once

#include <optional>
#include "map/Map.hpp"
#include "util/Contracts.hpp"

namespace ljn {

//a position or a speed, in pixels (or pixels per second)
struct Vec2 {
    float x{0.0f};
    float y{0.0f};
};

//every number that decides how the player feels, easy to tweak
struct PlayerParams {
    //size of the body, in pixels
    float width{24.0f};
    float height{48.0f};

    //horizontal movement: top speed and how fast the speed changes (pixels per second, per second)
    float runSpeed{240.0f};
    float groundAccel{2200.0f};
    float groundFriction{2600.0f};
    float airAccel{1500.0f};
    float airFriction{600.0f};

    //vertical movement
    float gravity{1700.0f};
    float maxFall{900.0f};

    //jump speed: the jump height is speed * speed / (2 * gravity), about 106 pixels with these numbers
    float jumpSpeed{600.0f};

    //gravity is multiplied by this while the player rises with the jump button released (short hop)
    float shortHopGravity{2.6f};

    //forgiveness: a jump still works this long after leaving a ledge, and a press this long before landing still counts
    float coyoteTime{0.10f};
    float jumpBuffer{0.12f};
};

//what the player asked for in this step
struct PlayerInput {
    //-1 left, 0 nothing, 1 right (analog values in between are fine)
    float move{0.0f};

    //the jump button went down in this step, and whether it is still held
    bool jumpPressed{false};
    bool jumpHeld{false};
};

//everything that changes while playing
struct PlayerState {
    //top-left corner of the body, and its speed
    Vec2 position;
    Vec2 velocity;

    bool onGround{false};

    //1 looking right, -1 looking left
    int facing{1};

    //time left to jump after leaving the ground, and time left of a jump press that is waiting for the ground
    float coyoteLeft{0.0f};
    float jumpBufferLeft{0.0f};
};

//a player standing still on the bottom edge of this cell, centered in it
[[nodiscard]] PlayerState spawnState(Cell cell, const PlayerParams& params = {}) noexcept;

//moves the player by dt seconds: input, gravity, jump and collision with the solid tiles
void step(PlayerState& state, const Map& map, const PlayerInput& input, const PlayerParams& params, float dt)
    LJN_PRE(dt > 0.0f);

//true if the player fell below the bottom of the map
[[nodiscard]] bool fellOutOfMap(const PlayerState& state, const Map& map) noexcept;

//index (in map.checkpoints()) of the first checkpoint the player touches, if any
//the touch area of a checkpoint is its cell and the two cells above it, so walking past it is enough
[[nodiscard]] std::optional<int> checkpointTouched(const PlayerState& state, const Map& map,
                                                   const PlayerParams& params) noexcept;

//the cell where the player appears after dying: the active checkpoint, or the spawn point, or a safe default
[[nodiscard]] Cell respawnCell(const Map& map, std::optional<int> activeCheckpoint) noexcept;

}  //namespace ljn
