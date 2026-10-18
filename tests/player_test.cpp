// player_test.cpp - checks the player's movement: gravity, running, walls, jumping, ledges, checkpoints and respawn.

#include <algorithm>
#include <cmath>
#include <print>
#include "check.hpp"
#include "map/Map.hpp"
#include "sim/Player.hpp"

using namespace ljn;

//the game runs 60 steps per second
constexpr float kDt = 1.0f / 60.0f;

//true if two numbers are closer than the tolerance
static bool near(float a, float b, float tolerance = 0.01f) {
    return std::fabs(a - b) <= tolerance;
}

//a map with one solid layer and one decoration layer that is not solid, and a 1x1 solid tile type
struct World {
    Map map;
    TileId block;
    int ground;
    int decor;
};

static World makeWorld(int width = 30, int height = 12) {
    Map map = Map::create(width, height).value();
    const int ground = map.addLayer({.name = "Ground", .solid = true}).value();
    const int decor = map.addLayer({.name = "Decor"}).value();
    const TileId block = map.addTileDef({.file = "block.png"}).value();
    return {std::move(map), block, ground, decor};
}

//a floor on the last row, from column first to column last
static void addFloor(World& world, int first, int last) {
    for (int x = first; x <= last; ++x) {
        CHECK(world.map.place(world.ground, {x, world.map.height() - 1}, world.block).has_value());
    }
}

//runs the player for some steps with the same input
static void run(PlayerState& state, const Map& map, const PlayerInput& input, int steps) {
    const PlayerParams params;
    for (int i = 0; i < steps; ++i) {
        step(state, map, input, params, kDt);
    }
}

//the top of the floor in pixels (the floor is on the last row)
static float floorTop(const Map& map) {
    return static_cast<float>((map.height() - 1) * kCellSize);
}

//a player that is placed in the air falls and lands exactly on the floor
static void testLanding() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const PlayerParams params;

    PlayerState state = spawnState({3, 5}, params);
    CHECK(!state.onGround);
    run(state, world.map, {}, 120);

    CHECK(state.onGround);
    CHECK(near(state.position.y, floorTop(world.map) - params.height));

    //standing still: it stays on the ground and does not sink or float
    const float before = state.position.y;
    run(state, world.map, {}, 60);
    CHECK(state.onGround && near(state.position.y, before));
}

//spawnState puts the feet on the bottom of the cell and the body in the middle of it
static void testSpawnState() {
    const PlayerParams params;
    CHECK(near(params.width, 56.0f) && near(params.height, 56.0f));

    //the hitbox is wider than a cell, so it sticks out the same amount on both sides: (32 - 56) / 2 = -12
    const PlayerState state = spawnState({4, 6}, params);
    CHECK(near(state.position.x, 4.0f * 32.0f - 12.0f));
    CHECK(near(state.position.y, 7.0f * 32.0f - 56.0f));
}

//running right speeds up to the top speed and stops at a wall, left stops at the edge of the map
static void testRunAndWalls() {
    World world = makeWorld();
    addFloor(world, 0, 29);

    //a wall of 3 blocks on column 20
    for (int y = 8; y <= 10; ++y) {
        CHECK(world.map.place(world.ground, {20, y}, world.block).has_value());
    }
    const PlayerParams params;
    PlayerState state = spawnState({2, 10}, params);
    run(state, world.map, {}, 5);
    CHECK(state.onGround);

    //after a short run it is fast but not faster than the top speed
    run(state, world.map, {.move = 1.0f}, 30);
    CHECK(state.velocity.x > 100.0f && state.velocity.x <= params.runSpeed + 0.01f);
    CHECK(state.facing == 1);

    //after a long run it stands against the wall
    run(state, world.map, {.move = 1.0f}, 180);
    CHECK(near(state.position.x + params.width, 20.0f * 32.0f));
    CHECK(state.velocity.x == 0.0f);

    //running left ends at the left edge of the map
    run(state, world.map, {.move = -1.0f}, 400);
    CHECK(near(state.position.x, 0.0f));
    CHECK(state.facing == -1);
}

//letting go of the direction slows the player down until it stops
static void testFriction() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    PlayerState state = spawnState({2, 10});
    run(state, world.map, {.move = 1.0f}, 30);
    CHECK(state.velocity.x > 100.0f);
    run(state, world.map, {}, 30);
    CHECK(state.velocity.x == 0.0f);
}

//the highest point of a jump, in pixels above the start
//the button is held until step releaseAt (a negative number means never released, 0 means a tap)
static float jumpHeight(const World& world, int releaseAt) {
    const PlayerParams params;
    PlayerState state = spawnState({3, 10}, params);
    run(state, world.map, {}, 5);
    CHECK(state.onGround);

    const float start = state.position.y;
    float highest = start;
    for (int i = 0; i < 120; ++i) {
        const bool held = releaseAt < 0 || i < releaseAt || i == 0;
        const PlayerInput input{.jumpPressed = i == 0, .jumpHeld = held};
        step(state, world.map, input, params, kDt);
        highest = std::min(highest, state.position.y);
    }

    //after two seconds the player is back on the floor
    CHECK(state.onGround && near(state.position.y, start));
    return start - highest;
}

//a held jump is 3 blocks high and a tap is 1.5 blocks, the numbers come from the params
static void testJump() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const PlayerParams params;

    CHECK(near(jumpHeight(world, -1), 96.0f, 1.0f));
    CHECK(near(jumpHeight(world, 0), 48.0f, 1.0f));
    CHECK(near(jumpHeight(world, -1), params.jumpHeight, 1.0f));
    CHECK(near(jumpHeight(world, 0), params.tapHeight, 1.0f));
}

//letting go in the middle of a jump gives something in between, and the rise slows down softly (no sudden stop)
static void testJumpRelease() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const PlayerParams params;

    //released later means higher
    const float early = jumpHeight(world, 4);
    const float late = jumpHeight(world, 12);
    CHECK(early > params.tapHeight && late > early && late < params.jumpHeight);

    //the step after the release changes the speed by about two gravities, it is not set to zero
    PlayerState state = spawnState({3, 10}, params);
    run(state, world.map, {}, 5);
    for (int i = 0; i < 8; ++i) {
        step(state, world.map, {.jumpPressed = i == 0, .jumpHeld = true}, params, kDt);
    }
    const float before = state.velocity.y;
    step(state, world.map, {.jumpHeld = false}, params, kDt);
    CHECK(before < -100.0f);
    CHECK(state.velocity.y < 0.0f);
    CHECK(near(state.velocity.y - before, params.gravity() * params.releaseGravityScale() * kDt, 1.0f));
}

//can the player run and jump from the floor onto a ledge that is n blocks high? the jump is held or a tap
static bool canClimb(int blocks, bool hold) {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const int ledgeTop = world.map.height() - 1 - blocks;
    for (int y = ledgeTop; y < world.map.height() - 1; ++y) {
        for (int x = 9; x <= 14; ++x) {
            CHECK(world.map.place(world.ground, {x, y}, world.block).has_value());
        }
    }

    const PlayerParams params;
    PlayerState state = spawnState({6, world.map.height() - 2}, params);
    run(state, world.map, {}, 5);
    for (int i = 0; i < 120; ++i) {
        //it runs for half a second and then stops, so it does not walk off the other side of the ledge
        const PlayerInput input{.move = i < 30 ? 1.0f : 0.0f, .jumpPressed = i == 0, .jumpHeld = hold || i == 0};
        step(state, world.map, input, params, kDt);
    }

    //standing on top of the ledge: the feet are on its top edge
    return state.onGround && near(state.position.y + params.height, static_cast<float>(ledgeTop * kCellSize));
}

//a tap clears one block, a held jump clears two, nothing clears four
static void testLedges() {
    CHECK(canClimb(1, false));
    CHECK(!canClimb(2, false));
    CHECK(canClimb(2, true));
    CHECK(!canClimb(4, true));
}

//a tile above the player stops the jump and the player falls back down
static void testCeiling() {
    World world = makeWorld();
    addFloor(world, 0, 29);

    //the underside of this block is at y = 8 * 32 + 32 = 288
    CHECK(world.map.place(world.ground, {3, 8}, world.block).has_value());
    const PlayerParams params;
    PlayerState state = spawnState({3, 10}, params);
    run(state, world.map, {}, 5);

    float highest = state.position.y;
    for (int i = 0; i < 90; ++i) {
        step(state, world.map, {.jumpPressed = i == 0, .jumpHeld = true}, params, kDt);
        highest = std::min(highest, state.position.y);
    }
    CHECK(highest >= 288.0f - 0.01f);
    CHECK(state.onGround);
}

//tiles on a layer that is not solid do not block anything
static void testNonSolidLayer() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    CHECK(world.map.place(world.decor, {5, 10}, world.block).has_value());
    const PlayerParams params;

    PlayerState state = spawnState({3, 10}, params);
    run(state, world.map, {.move = 1.0f}, 120);
    CHECK(state.position.x > 6.0f * 32.0f);
}

//a jump still works a moment after walking off a ledge, but not much later
static void testCoyoteTime() {
    World world = makeWorld();
    addFloor(world, 0, 9);
    const PlayerParams params;

    //walk right until the feet leave the ledge, step by step
    const auto walkOff = [&]() {
        PlayerState state = spawnState({2, 10}, params);
        run(state, world.map, {}, 5);
        for (int i = 0; i < 300 && state.onGround; ++i) {
            step(state, world.map, {.move = 1.0f}, params, kDt);
        }
        CHECK(!state.onGround);
        return state;
    };

    //right after leaving, the jump works
    PlayerState early = walkOff();
    step(early, world.map, {.move = 1.0f, .jumpPressed = true, .jumpHeld = true}, params, kDt);
    CHECK(early.velocity.y < -params.jumpSpeed() * 0.5f);

    //a quarter of a second later it is too late
    PlayerState late = walkOff();
    run(late, world.map, {.move = 1.0f}, 15);
    step(late, world.map, {.move = 1.0f, .jumpPressed = true, .jumpHeld = true}, params, kDt);
    CHECK(late.velocity.y > 0.0f);
}

//a press just before landing is remembered and the jump happens on landing, a very early press is forgotten
static void testJumpBuffer() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const PlayerParams params;

    //the number of steps from a small height until landing
    const auto landingStep = [&]() {
        PlayerState state = spawnState({3, 9}, params);
        for (int i = 0; i < 200; ++i) {
            step(state, world.map, {}, params, kDt);
            if (state.onGround) {
                return i;
            }
        }
        return -1;
    };
    const int landing = landingStep();
    CHECK(landing > 5);

    //pressing 3 steps before landing: the player jumps when it lands
    PlayerState near_ = spawnState({3, 9}, params);
    bool jumped = false;
    for (int i = 0; i <= landing + 3; ++i) {
        step(near_, world.map, {.jumpPressed = i == landing - 3, .jumpHeld = true}, params, kDt);
        jumped = jumped || near_.velocity.y < -params.jumpSpeed() * 0.5f;
    }
    CHECK(jumped);

    //pressing at the very start of the fall: the press is gone before landing
    PlayerState far_ = spawnState({3, 9}, params);
    bool farJumped = false;
    for (int i = 0; i <= landing + 3; ++i) {
        step(far_, world.map, {.jumpPressed = i == 0, .jumpHeld = true}, params, kDt);
        farJumped = farJumped || far_.velocity.y < -params.jumpSpeed() * 0.5f;
    }
    CHECK(!farJumped);
}

//one press gives one jump, even if the button stays down
static void testNoAutoJump() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    const PlayerParams params;
    PlayerState state = spawnState({3, 10}, params);
    run(state, world.map, {}, 5);

    //press once and hold: after landing the player does not jump again by itself
    for (int i = 0; i < 240; ++i) {
        step(state, world.map, {.jumpPressed = i == 0, .jumpHeld = true}, params, kDt);
    }
    CHECK(state.onGround && near(state.velocity.y, 0.0f, 40.0f));
}

//falling below the map is detected
static void testFellOutOfMap() {
    World world = makeWorld();
    addFloor(world, 0, 5);
    const PlayerParams params;

    PlayerState state = spawnState({10, 5}, params);
    CHECK(!fellOutOfMap(state, world.map));
    run(state, world.map, {}, 200);
    CHECK(fellOutOfMap(state, world.map));
}

//walking through the area above a checkpoint touches it, and the respawn cell follows the rules
static void testCheckpointsAndRespawn() {
    World world = makeWorld();
    addFloor(world, 0, 29);
    CHECK(world.map.setSpawn({2, 10}));
    CHECK(world.map.addCheckpoint({12, 10}));
    CHECK(world.map.addCheckpoint({22, 10}));
    const PlayerParams params;

    //nothing is touched at the spawn
    PlayerState state = spawnState({2, 10}, params);
    CHECK(!checkpointTouched(state, world.map, params).has_value());

    //standing in the first checkpoint cell touches it, and so does being in the air above it
    CHECK(checkpointTouched(spawnState({12, 10}, params), world.map, params) == 0);
    CHECK(checkpointTouched(spawnState({22, 8}, params), world.map, params) == 1);

    //running through the level reaches the first checkpoint
    int reached = -1;
    for (int i = 0; i < 600 && reached < 0; ++i) {
        step(state, world.map, {.move = 1.0f}, params, kDt);
        reached = checkpointTouched(state, world.map, params).value_or(-1);
    }
    CHECK(reached == 0);

    //respawn: no checkpoint -> spawn, checkpoint -> that checkpoint, wrong index -> spawn
    CHECK(respawnCell(world.map, std::nullopt) == Cell{2, 10});
    CHECK(respawnCell(world.map, 1) == Cell{22, 10});
    CHECK(respawnCell(world.map, 7) == Cell{2, 10});

    //a map without spawn has a safe default
    World bare = makeWorld();
    CHECK(respawnCell(bare.map, std::nullopt) == Cell{1, 1});
}

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testLanding();
    testSpawnState();
    testRunAndWalls();
    testFriction();
    testJump();
    testJumpRelease();
    testLedges();
    testCeiling();
    testNonSolidLayer();
    testCoyoteTime();
    testJumpBuffer();
    testNoAutoJump();
    testFellOutOfMap();
    testCheckpointsAndRespawn();
    std::println("all player tests passed");
}
