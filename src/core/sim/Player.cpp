// Player.cpp - implementation of the player movement and collision.

#include "sim/Player.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {
namespace {

//edges that only touch a tile are not inside it, this small gap makes that true even with rounding errors
constexpr float kEpsilon = 0.001f;

constexpr auto kCell = static_cast<float>(kCellSize);

//moves a value toward a target by at most delta, never past it
float approach(float value, float target, float delta) noexcept {
    if (value < target) {
        return std::min(value + delta, target);
    }
    return std::max(value - delta, target);
}

//index of the cell that contains a pixel position
int cellOf(float pixel) noexcept {
    return static_cast<int>(std::floor(pixel / kCell));
}

//true if any solid cell overlaps the rectangle
bool touchesSolid(const Map& map, float left, float top, float width, float height) noexcept {
    const int firstX = cellOf(left);
    const int lastX = cellOf(left + width - kEpsilon);
    const int firstY = cellOf(top);
    const int lastY = cellOf(top + height - kEpsilon);

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            if (map.isSolid({x, y})) {
                return true;
            }
        }
    }
    return false;
}

//horizontal speed: speeds up toward the wanted speed, and slows down when nothing is pressed
void updateHorizontalSpeed(PlayerState& state, const PlayerInput& input, const PlayerParams& params, float dt) noexcept {
    const float wanted = std::clamp(input.move, -1.0f, 1.0f) * params.runSpeed;
    const bool pressing = input.move != 0.0f;

    float rate = 0.0f;
    if (pressing) {
        rate = state.onGround ? params.groundAccel : params.airAccel;
    } else {
        rate = state.onGround ? params.groundFriction : params.airFriction;
    }
    state.velocity.x = approach(state.velocity.x, wanted, rate * dt);
}

//jump: coyote time and the jump buffer make the timing forgiving
void updateJump(PlayerState& state, const PlayerInput& input, const PlayerParams& params, float dt) noexcept {
    //a press is remembered for a short time, the ground refills the coyote time
    state.jumpBufferLeft = input.jumpPressed ? params.jumpBuffer : std::max(0.0f, state.jumpBufferLeft - dt);
    state.coyoteLeft = state.onGround ? params.coyoteTime : std::max(0.0f, state.coyoteLeft - dt);

    if (state.jumpBufferLeft > 0.0f && state.coyoteLeft > 0.0f) {
        state.velocity.y = -params.jumpSpeed();
        state.onGround = false;

        //both timers are used up, so one press gives one jump
        state.jumpBufferLeft = 0.0f;
        state.coyoteLeft = 0.0f;
    }
}

//gravity has three strengths: normal while rising with the button held, stronger while rising with the button
//released (the jump ends softly, the speed is not cut at once), and the fall one while going down
void applyGravity(PlayerState& state, const PlayerInput& input, const PlayerParams& params, float dt) noexcept {
    float gravity = params.gravity();
    if (state.velocity.y < 0.0f) {
        if (!input.jumpHeld) {
            gravity *= params.releaseGravityScale();
        }
    } else {
        gravity *= params.fallGravityScale;
    }
    state.velocity.y = std::min(state.velocity.y + gravity * dt, params.maxFall);
}

//moves along x and stops at the first wall or at the edge of the map
void moveX(PlayerState& state, const Map& map, const PlayerParams& params, float dt) noexcept {
    float x = state.position.x + state.velocity.x * dt;

    //the left and right edges of the map are walls
    const float maxX = static_cast<float>(map.width()) * kCell - params.width;
    if (x < 0.0f || x > maxX) {
        x = std::clamp(x, 0.0f, maxX);
        state.velocity.x = 0.0f;
    }

    //the player was free before, and the step is smaller than a cell, so the wall is on the side we move to
    if (touchesSolid(map, x, state.position.y, params.width, params.height)) {
        if (state.velocity.x > 0.0f) {
            x = static_cast<float>(cellOf(x + params.width - kEpsilon)) * kCell - params.width;
        } else if (state.velocity.x < 0.0f) {
            x = static_cast<float>(cellOf(x) + 1) * kCell;
        }
        state.velocity.x = 0.0f;
    }
    state.position.x = x;
}

//moves along y, lands on floors and bumps into ceilings
void moveY(PlayerState& state, const Map& map, const PlayerParams& params, float dt) noexcept {
    float y = state.position.y + state.velocity.y * dt;
    state.onGround = false;

    if (touchesSolid(map, state.position.x, y, params.width, params.height)) {
        if (state.velocity.y > 0.0f) {
            //falling: stand on top of the tile
            y = static_cast<float>(cellOf(y + params.height - kEpsilon)) * kCell - params.height;
            state.onGround = true;
        } else {
            //rising: stop under the tile
            y = static_cast<float>(cellOf(y) + 1) * kCell;
        }
        state.velocity.y = 0.0f;
    }
    state.position.y = y;
}

}  //anonymous namespace

PlayerState spawnState(Cell cell, const PlayerParams& params) noexcept {
    PlayerState state;
    state.position.x = static_cast<float>(cell.x) * kCell + (kCell - params.width) * 0.5f;
    state.position.y = static_cast<float>(cell.y + 1) * kCell - params.height;
    return state;
}

void step(PlayerState& state, const Map& map, const PlayerInput& input, const PlayerParams& params, float dt) {
    updateHorizontalSpeed(state, input, params, dt);
    updateJump(state, input, params, dt);
    applyGravity(state, input, params, dt);
    moveX(state, map, params, dt);
    moveY(state, map, params, dt);

    //the player looks where it goes
    if (input.move > 0.0f) {
        state.facing = 1;
    } else if (input.move < 0.0f) {
        state.facing = -1;
    }
}

bool fellOutOfMap(const PlayerState& state, const Map& map) noexcept {
    //a bit below the last row, so a quick dip at the edge is not a death
    return state.position.y > static_cast<float>(map.height()) * kCell + kCell * 2.0f;
}

std::optional<int> checkpointTouched(const PlayerState& state, const Map& map, const PlayerParams& params) noexcept {
    const auto checkpoints = map.checkpoints();
    for (std::size_t i = 0; i < checkpoints.size(); ++i) {
        //the touch area: the checkpoint cell and the two cells above it
        const float left = static_cast<float>(checkpoints[i].x) * kCell;
        const float top = static_cast<float>(checkpoints[i].y - 2) * kCell;
        const float right = left + kCell;
        const float bottom = top + 3.0f * kCell;

        const bool overlapsX = state.position.x < right && state.position.x + params.width > left;
        const bool overlapsY = state.position.y < bottom && state.position.y + params.height > top;
        if (overlapsX && overlapsY) {
            return static_cast<int>(i);
        }
    }
    return std::nullopt;
}

Cell respawnCell(const Map& map, std::optional<int> activeCheckpoint) noexcept {
    const auto checkpoints = map.checkpoints();
    if (activeCheckpoint && *activeCheckpoint >= 0 && static_cast<std::size_t>(*activeCheckpoint) < checkpoints.size()) {
        return checkpoints[static_cast<std::size_t>(*activeCheckpoint)];
    }
    if (map.hasSpawn()) {
        return map.spawn();
    }
    return {1, 1};
}

}  //namespace ljn
