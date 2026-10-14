// NaturalSort.hpp - orders file names the way people do: "frame2" comes before "frame10".

#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string_view>

namespace ljn {
namespace detail {

//a number written with digits, without the zeros at the start
inline std::string_view withoutLeadingZeros(std::string_view digits) noexcept {
    const auto first = digits.find_first_not_of('0');
    return first == std::string_view::npos ? std::string_view{} : digits.substr(first);
}

inline bool isDigit(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

}  //namespace detail

//true if a goes before b
[[nodiscard]] inline bool naturalLess(std::string_view a, std::string_view b) noexcept {
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (detail::isDigit(a[i]) && detail::isDigit(b[j])) {
            //two numbers: the one with more digits is bigger, same length compares digit by digit
            std::size_t endA = i;
            while (endA < a.size() && detail::isDigit(a[endA])) {
                ++endA;
            }
            std::size_t endB = j;
            while (endB < b.size() && detail::isDigit(b[endB])) {
                ++endB;
            }
            const auto numberA = detail::withoutLeadingZeros(a.substr(i, endA - i));
            const auto numberB = detail::withoutLeadingZeros(b.substr(j, endB - j));
            if (numberA.size() != numberB.size()) {
                return numberA.size() < numberB.size();
            }
            if (numberA != numberB) {
                return numberA < numberB;
            }
            i = endA;
            j = endB;
        } else {
            //two normal characters
            if (a[i] != b[j]) {
                return a[i] < b[j];
            }
            ++i;
            ++j;
        }
    }

    //one name is the start of the other: the shorter one goes first
    return (a.size() - i) < (b.size() - j);
}

}  //namespace ljn
