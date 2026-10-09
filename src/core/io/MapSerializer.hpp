// MapSerializer.hpp - saves a Map as json text or file, and loads it back.

#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include "map/Map.hpp"

namespace ljn {

//version of the file format, raise it whenever the format changes
inline constexpr int kFormatVersion = 1;

//everything that can go wrong when saving or loading
enum class IoError : std::uint8_t {
    FileOpenFailed,
    FileWriteFailed,
    ParseFailed,
    UnsupportedVersion,
    CellSizeMismatch,
    InvalidFormat,
    InvalidMap,
    InvalidTile,
    InvalidPlacement,
    InvalidObject,
};

//turns the map into json text
[[nodiscard]] std::string toJson(const Map& map);

//builds a map from json text, or says what is wrong with it
[[nodiscard]] std::expected<Map, IoError> fromJson(std::string_view text);

//writes the map to a file
std::expected<void, IoError> saveToFile(const Map& map, const std::filesystem::path& path);

//reads a map from a file
[[nodiscard]] std::expected<Map, IoError> loadFromFile(const std::filesystem::path& path);

//short readable text for an error, for the editor ui
[[nodiscard]] std::string_view describe(IoError error) noexcept;

}  //namespace ljn
