// ButtonSheet.hpp - the maths of a sprite sheet of menu buttons: where each frame is in the image, which frame to show,
// and where each button is on the screen. no drawing here, so it can be tested without a window.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ljn {

//a rectangle in pixels (the core does not use raylib types)
struct PixelRect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    [[nodiscard]] constexpr bool contains(float px, float py) const noexcept {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

//how the sheet is built: one row for each button, and the columns are the frames
//the idle frames come first, then the hover frames (the look when the mouse or the cursor is on the button)
struct SheetLayout {
    int rows{5};
    int idleFrames{2};
    int hoverFrames{2};

    [[nodiscard]] constexpr int columns() const noexcept { return idleFrames + hoverFrames; }
};

//how long each frame stays on screen
struct SheetAnimation {
    double idleFrameSeconds{1.2};
    double hoverFrameSeconds{0.6};
};

enum class ButtonLook { Idle, Hover };

//the cell of the image that holds one frame, in pixels of the image
//the image is cut in cells of the same size (a few leftover pixels at the right or bottom edge are ignored)
[[nodiscard]] constexpr PixelRect frameSource(int imageWidth, int imageHeight, const SheetLayout& layout, int row,
                                              int column) noexcept {
    const int cellWidth = imageWidth / std::max(1, layout.columns());
    const int cellHeight = imageHeight / std::max(1, layout.rows);
    return {static_cast<float>(column * cellWidth), static_cast<float>(row * cellHeight),
            static_cast<float>(cellWidth), static_cast<float>(cellHeight)};
}

//the column to show at this time, the frames repeat forever
[[nodiscard]] inline int frameColumn(const SheetLayout& layout, const SheetAnimation& animation, ButtonLook look,
                                     double seconds) noexcept {
    const bool hover = look == ButtonLook::Hover;
    const int frames = std::max(1, hover ? layout.hoverFrames : layout.idleFrames);
    const double frameSeconds = hover ? animation.hoverFrameSeconds : animation.idleFrameSeconds;

    const auto step = static_cast<long long>(std::floor(seconds / std::max(0.001, frameSeconds)));
    const int frame = static_cast<int>(((step % frames) + frames) % frames);
    return hover ? layout.idleFrames + frame : frame;
}

//buttons stacked in a column, all with the same height (pitch) and centered on one x
struct ButtonStack {
    float centerX{480.0f};
    float top{296.0f};

    //the distance from the center of one button to the next
    float pitch{46.0f};

    //how wide the area that reacts to the mouse is
    float hitWidth{160.0f};

    //the area of the button at this position in the list, areas touch each other, so there are no gaps
    [[nodiscard]] constexpr PixelRect slot(std::size_t index) const noexcept {
        return {centerX - hitWidth * 0.5f, top + pitch * static_cast<float>(index), hitWidth, pitch};
    }

    //the center of the button, where its picture is centered
    [[nodiscard]] constexpr float centerY(std::size_t index) const noexcept {
        return top + pitch * (static_cast<float>(index) + 0.5f);
    }

    //the position in the list of the button under a point, -1 if there is none
    [[nodiscard]] constexpr int indexAt(float px, float py, std::size_t count) const noexcept {
        if (pitch <= 0.0f || py < top || px < centerX - hitWidth * 0.5f || px >= centerX + hitWidth * 0.5f) {
            return -1;
        }
        const auto index = static_cast<std::size_t>((py - top) / pitch);
        return index < count ? static_cast<int>(index) : -1;
    }
};

}  //namespace ljn
