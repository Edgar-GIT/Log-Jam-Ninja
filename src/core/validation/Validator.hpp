// Validator.hpp - checks a map against the level rules before it can be saved.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include "map/Map.hpp"
#include "util/Contracts.hpp"

namespace ljn {

//how bad a problem is: errors block saving, warnings do not
enum class Severity : std::uint8_t { Warning, Error };

//every kind of problem the validator can find
enum class IssueCode : std::uint8_t {
    EmptyColumn,
    StrayTiles,
    MissingSpawn,
    SpawnInsideSolid,
    SpawnNoGround,
    CheckpointInsideSolid,
    CheckpointNoGround,
};

//one problem found: how bad it is, what kind and which cell to highlight
struct Issue {
    Severity severity;
    IssueCode code;
    Cell cell;
};

//settings for the rules, easy to tweak
struct Rules {
    //a group of connected cells smaller than this is treated as a mistake
    int minStructureCells = 3;
};

//the result of validating a map
struct ValidationReport {
    std::vector<Issue> issues;

    //true if at least one issue is an error
    [[nodiscard]] bool hasErrors() const noexcept {
        return std::ranges::any_of(issues, [](const Issue& i) { return i.severity == Severity::Error; });
    }

    //how many issues have this code
    [[nodiscard]] std::size_t count(IssueCode code) const noexcept {
        return static_cast<std::size_t>(
            std::ranges::count_if(issues, [code](const Issue& i) { return i.code == code; }));
    }
};

//runs every rule on the map
[[nodiscard]] ValidationReport validate(const Map& map, const Rules& rules = {})
    LJN_PRE(rules.minStructureCells >= 1);

//short readable text for an issue code, for the editor ui
[[nodiscard]] std::string_view describe(IssueCode code) noexcept;

}  //namespace ljn
