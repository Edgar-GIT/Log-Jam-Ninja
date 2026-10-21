// TimeFormat.cpp - implementation of formatTime.

#include "util/TimeFormat.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace ljn {

std::string formatTime(double seconds) {
    //work in hundredths of a second, so the rounding is done only once
    const auto hundredths = static_cast<long long>(std::floor(std::max(seconds, 0.0) * 100.0));
    const long long minutes = hundredths / 6000;
    const long long rest = hundredths % 6000;
    return std::format("{}:{:02}.{:02}", minutes, rest / 100, rest % 100);
}

}  //namespace ljn
