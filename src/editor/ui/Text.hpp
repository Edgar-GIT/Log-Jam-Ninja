// Text.hpp - small text helpers for the editor ui.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ljn {

//cuts long text from the left, keeping the end: "...ong_name.png"
inline std::string shorten(std::string_view text, std::size_t maxLength) {
    if (text.size() <= maxLength) {
        return std::string(text);
    }
    return "..." + std::string(text.substr(text.size() - (maxLength - 3)));
}

}  //namespace ljn
