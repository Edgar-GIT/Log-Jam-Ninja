// Overlays.cpp - implementation of the flags and the problem boxes.

#include "Overlays.hpp"

#include <string>

namespace ljn {
namespace {

constexpr Color kSpawnColor{70, 210, 100, 255};
constexpr Color kCheckpointColor{70, 170, 255, 255};
constexpr Color kPoleColor{235, 235, 240, 255};

constexpr Color kErrorFill{255, 50, 50, 90};
constexpr Color kErrorLine{255, 70, 70, 255};
constexpr Color kWarningFill{255, 200, 60, 80};
constexpr Color kWarningLine{255, 210, 70, 255};

//a small flag inside one cell, with a short text under it
void drawFlag(Cell cell, Color color, const std::string& label, float alpha) {
    const auto size = static_cast<float>(kCellSize);
    const float x = static_cast<float>(cell.x) * size;
    const float y = static_cast<float>(cell.y) * size;

    //a faint background so the cell is easy to see, then the pole and the cloth
    DrawRectangleRec({x, y, size, size}, Fade(color, 0.20f * alpha));
    DrawRectangleRec({x + 6.0f, y + 3.0f, 3.0f, 26.0f}, Fade(kPoleColor, alpha));
    DrawTriangle({x + 9.0f, y + 3.0f}, {x + 9.0f, y + 15.0f}, {x + 27.0f, y + 9.0f}, Fade(color, alpha));

    if (!label.empty()) {
        DrawText(label.c_str(), static_cast<int>(x) + 12, static_cast<int>(y) + 16, 14, Fade(kPoleColor, alpha));
    }
}

}  //anonymous namespace

void drawSpawn(Cell cell, float alpha) {
    drawFlag(cell, kSpawnColor, "S", alpha);
}

void drawCheckpoint(Cell cell, int number, float alpha) {
    drawFlag(cell, kCheckpointColor, number > 0 ? std::to_string(number) : std::string{}, alpha);
}

void drawMarkers(const Map& map) {
    if (map.hasSpawn()) {
        drawSpawn(map.spawn(), 1.0f);
    }

    //checkpoints are sorted from left to right, so the number is the order the player reaches them
    int number = 1;
    for (const Cell& cell : map.checkpoints()) {
        drawCheckpoint(cell, number, 1.0f);
        ++number;
    }
}

void drawIssues(const ValidationReport& report, float zoom) {
    const auto size = static_cast<float>(kCellSize);
    for (const Issue& issue : report.issues) {
        const bool isError = issue.severity == Severity::Error;
        const Rectangle rect{static_cast<float>(issue.cell.x) * size, static_cast<float>(issue.cell.y) * size,
                             size, size};
        DrawRectangleRec(rect, isError ? kErrorFill : kWarningFill);
        DrawRectangleLinesEx(rect, 2.0f / zoom, isError ? kErrorLine : kWarningLine);
    }
}

}  //namespace ljn
