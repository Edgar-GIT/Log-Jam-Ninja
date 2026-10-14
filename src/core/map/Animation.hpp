// Animation.hpp - which frame of an animated tile is shown at a given time.

#pragma once

#include <cstddef>
#include <string>
#include "map/Map.hpp"
#include "util/Contracts.hpp"

namespace ljn {

//the frame to show after this many seconds, 0 for a tile that does not move
//every tile of the same type shows the same frame at the same time, so no state is kept per tile
[[nodiscard]] inline std::size_t frameAt(const TileDef& def, double seconds) noexcept LJN_PRE(seconds >= 0.0) {
    if (!def.isAnimated()) {
        return 0;
    }
    const auto ticks = static_cast<long long>(seconds * 1000.0) / def.frameMs;
    return static_cast<std::size_t>(ticks) % def.frames.size();
}

//the image file to draw for this tile at this time
[[nodiscard]] inline const std::string& currentFile(const TileDef& def, double seconds) noexcept
    LJN_PRE(seconds >= 0.0) {
    return def.isAnimated() ? def.frames[frameAt(def, seconds)] : def.file;
}

}  //namespace ljn
