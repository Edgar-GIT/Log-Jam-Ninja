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

//the game logic always runs in steps of this many seconds (the jump maths below needs to know it)
inline constexpr float kLogicStep = 1.0f / 60.0f;

//every number that decides how the player feels, easy to tweak
struct PlayerParams {
    //size of the hitbox, in pixels (the sprite is 64x64, the hitbox sits inside it, feet on the same line)
    float width{56.0f};
    float height{56.0f};

    //horizontal movement: top speed and how fast the speed changes (pixels per second, per second)
    float runSpeed{240.0f};
    float groundAccel{2200.0f};
    float groundFriction{2600.0f};
    float airAccel{1500.0f};
    float airFriction{600.0f};

    //jump, described by what the player sees: how high it goes and how long it takes to reach the top
    //jumpHeight is with the button held, tapHeight is a tap (button released at once), both in pixels
    //(3 blocks = 96 and 1.5 blocks = 48 with blocks of 32 pixels)
    float jumpHeight{96.0f};
    float tapHeight{48.0f};
    float apexTime{0.34f};

    //the fall: the maximum speed, and how much stronger gravity is while falling
    float maxFall{900.0f};
    float fallGravityScale{1.0f};

    //forgiveness: a jump still works this long after leaving a ledge, and a press this long before landing still counts
    float coyoteTime{0.10f};
    float jumpBuffer{0.12f};

    //the numbers the physics really uses, worked out from the ones above

    //a step of the simulation moves the player a bit less than the smooth curve, so the curve is made a little
    //taller to make the jump reach exactly jumpHeight
    [[nodiscard]] constexpr float curveHeight() const noexcept { return jumpHeight / (1.0f - kLogicStep / apexTime); }

    //upward gravity (pixels per second, per second) and the speed at the start of the jump
    [[nodiscard]] constexpr float gravity() const noexcept { return 2.0f * curveHeight() / (apexTime * apexTime); }
    [[nodiscard]] constexpr float jumpSpeed() const noexcept { return 2.0f * curveHeight() / apexTime; }

    //gravity is multiplied by this while rising with the button released, so a tap ends at tapHeight
    //(the first step of a jump still has normal gravity, so a little height is taken off the target)
    [[nodiscard]] constexpr float releaseGravityScale() const noexcept {
        return jumpHeight / (tapHeight - 0.25f * kLogicStep * jumpSpeed());
    }
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
