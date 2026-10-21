// TimeFormat.hpp - times in seconds written the way a speedrun shows them.

#pragma once

#include <string>

namespace ljn {

//83.456 seconds -> "1:23.45", 5.2 -> "0:05.20", negative numbers count as 0
[[nodiscard]] std::string formatTime(double seconds);

}  //namespace ljn
