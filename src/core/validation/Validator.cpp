// Validator.cpp - implementation of the map rules declared in Validator.hpp.

#include "validation/Validator.hpp"

#include <array>

namespace ljn {
namespace {

//the four cells next to a cell: right, left, down, up
constexpr std::array<Cell, 4> kNeighbours{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

//a column with no tile at all leaves a black strip, so it is an error
void checkEmptyColumns(const Map& map, ValidationReport& report) {
    const auto grid = map.grid();
    std::vector<std::uint8_t> filled(grid.extent(1), 0);

    //mark every column that has at least one tile
    for (std::size_t y = 0; y < grid.extent(0); ++y) {
        for (std::size_t x = 0; x < grid.extent(1); ++x) {
            if (grid[y, x].id != kEmptyTile) {
                filled[x] = 1;
            }
        }
    }

    //report the columns that were never marked
    for (std::size_t x = 0; x < filled.size(); ++x) {
        if (filled[x] == 0) {
            report.issues.push_back(
                {Severity::Error, IssueCode::EmptyColumn, {static_cast<std::int32_t>(x), 0}});
        }
    }
}

//groups of connected cells smaller than the minimum are probably mistakes
void checkStrayTiles(const Map& map, int minCells, ValidationReport& report) {
    const auto width = static_cast<std::size_t>(map.width());
    std::vector<std::uint8_t> visited(width * static_cast<std::size_t>(map.height()), 0);
    std::vector<Cell> stack;
    std::vector<Cell> group;

    //position of a cell in the visited array
    const auto indexOf = [width](Cell c) {
        return static_cast<std::size_t>(c.y) * width + static_cast<std::size_t>(c.x);
    };

    for (std::int32_t y = 0; y < map.height(); ++y) {
        for (std::int32_t x = 0; x < map.width(); ++x) {
            const Cell start{x, y};
            if (visited[indexOf(start)] != 0 || map.tile(start) == kEmptyTile) {
                continue;
            }

            //flood fill: collect every cell connected to this one
            group.clear();
            visited[indexOf(start)] = 1;
            stack.push_back(start);

            while (!stack.empty()) {
                const Cell current = stack.back();
                stack.pop_back();
                group.push_back(current);

                for (const Cell step : kNeighbours) {
                    const Cell next{current.x + step.x, current.y + step.y};
                    if (!map.inBounds(next) || visited[indexOf(next)] != 0
                        || map.tile(next) == kEmptyTile) {
                        continue;
                    }
                    visited[indexOf(next)] = 1;
                    stack.push_back(next);
                }
            }

            //a group that is too small gets reported cell by cell
            if (static_cast<int>(group.size()) < minCells) {
                for (const Cell c : group) {
                    report.issues.push_back({Severity::Error, IssueCode::StrayTiles, c});
                }
            }
        }
    }
}

//true if there is a solid cell somewhere below this one, in the same column
bool hasSolidBelow(const Map& map, Cell c) {
    for (std::int32_t y = c.y + 1; y < map.height(); ++y) {
        if (map.isSolid({c.x, y})) {
            return true;
        }
    }
    return false;
}

//the map needs a spawn, and it must not be inside a block or floating over nothing
void checkSpawn(const Map& map, ValidationReport& report) {
    if (!map.hasSpawn()) {
        report.issues.push_back({Severity::Error, IssueCode::MissingSpawn, {0, 0}});
        return;
    }

    const Cell spawn = map.spawn();
    if (map.isSolid(spawn)) {
        report.issues.push_back({Severity::Error, IssueCode::SpawnInsideSolid, spawn});
    } else if (!hasSolidBelow(map, spawn)) {
        report.issues.push_back({Severity::Error, IssueCode::SpawnNoGround, spawn});
    }
}

//checkpoints must not be inside a block, and floating ones only get a warning
void checkCheckpoints(const Map& map, ValidationReport& report) {
    for (const Cell c : map.checkpoints()) {
        if (map.isSolid(c)) {
            report.issues.push_back({Severity::Error, IssueCode::CheckpointInsideSolid, c});
        } else if (!hasSolidBelow(map, c)) {
            report.issues.push_back({Severity::Warning, IssueCode::CheckpointNoGround, c});
        }
    }
}

}  //anonymous namespace

//runs every rule and collects all the problems in one report
ValidationReport validate(const Map& map, const Rules& rules) {
    ValidationReport report;
    checkEmptyColumns(map, report);
    checkStrayTiles(map, rules.minStructureCells, report);
    checkSpawn(map, report);
    checkCheckpoints(map, report);
    return report;
}

//maps each issue code to a short text
std::string_view describe(IssueCode code) noexcept {
    switch (code) {
        case IssueCode::EmptyColumn:           return "Column is completely empty";
        case IssueCode::StrayTiles:            return "Loose tiles: group is too small";
        case IssueCode::MissingSpawn:          return "Map has no spawn point";
        case IssueCode::SpawnInsideSolid:      return "Spawn point is inside a solid tile";
        case IssueCode::SpawnNoGround:         return "Spawn point has no ground below";
        case IssueCode::CheckpointInsideSolid: return "Checkpoint is inside a solid tile";
        case IssueCode::CheckpointNoGround:    return "Checkpoint has no ground below";
    }
    return "Unknown issue";
}

}  //namespace ljn
