// ButtonSheet.hpp - the maths of a sprite sheet of menu buttons: where each frame is in the image, which frame to show,
// and where each button is on the screen. no drawing here, so it can be tested without a window.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

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
//the sheet of the game has one frame of each, so a button only changes when the cursor is on it
struct SheetLayout {
    int rows{5};
    int idleFrames{1};
    int hoverFrames{1};

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

//buttons in rows: the first row is at the top, each row can have more than one button side by side
struct ButtonStack {
    //the x of the middle of the column
    float centerX{480.0f};

    //the y of the top of the first row
    float top{296.0f};

    //the distance from the center of one row to the next
    float pitch{46.0f};

    //how wide the area that reacts to the mouse is
    float hitWidth{160.0f};

    //the space between two buttons of the same row
    float gap{20.0f};

    //how many buttons each row has, from the top row to the bottom one
    std::vector<int> rows{1};

    //where a button is: the row, its place in that row and how many buttons that row has
    struct Place {
        int row{0};
        int column{0};
        int count{1};
    };

    //the row of a button, its place in the row and how many buttons the row has.
    //the last row of the layout takes what is left, and a button that does not fit goes on a row of its own
    [[nodiscard]] Place placeOf(std::size_t index, std::size_t count) const noexcept {
        int used = 0;
        for (int row = 0; row < static_cast<int>(rows.size()); ++row) {
            const int want = std::max(1, rows[static_cast<std::size_t>(row)]);
            const int take = std::min(want, static_cast<int>(count) - used);
            if (take <= 0) {
                break;
            }
            if (index < static_cast<std::size_t>(used + take)) {
                return {row, static_cast<int>(index) - used, take};
            }
            used += take;
        }
        const int extra = static_cast<int>(index) - used;
        return {static_cast<int>(rows.size()) + extra, 0, 1};
    }

    //the area of the button at this position in the list, areas touch each other, so there are no gaps
    [[nodiscard]] PixelRect slot(std::size_t index, std::size_t count) const noexcept {
        const Place place = placeOf(index, count);
        return {centerXOf(index, count) - hitWidth * 0.5f, top + pitch * static_cast<float>(place.row), hitWidth,
                pitch};
    }

    //the center of the button, where its picture is centered
    [[nodiscard]] float centerY(std::size_t index, std::size_t count) const noexcept {
        const Place place = placeOf(index, count);
        return top + pitch * (static_cast<float>(place.row) + 0.5f);
    }

    //the x of the center of the button: the row is centered on centerX
    [[nodiscard]] float centerXOf(std::size_t index, std::size_t count) const noexcept {
        const Place place = placeOf(index, count);
        const float step = hitWidth + gap;
        const float offset = static_cast<float>(place.column) - static_cast<float>(place.count - 1) * 0.5f;
        return centerX + offset * step;
    }

    //the first index of a row and how many buttons it has (no buttons past the last row)
    struct RowInfo {
        int first{0};
        int count{0};
    };

    [[nodiscard]] RowInfo rowInfo(int row, std::size_t count) const noexcept {
        int first = 0;
        for (int r = 0; r <= row; ++r) {
            //a row past the layout has one button, like the extra rows of placeOf
            const int want = r < static_cast<int>(rows.size()) ? std::max(1, rows[static_cast<std::size_t>(r)]) : 1;
            const int left = static_cast<int>(count) - first;
            if (left <= 0) {
                return {0, 0};
            }
            const int take = std::min(want, left);
            if (r == row) {
                return {first, take};
            }
            first += take;
        }
        return {0, 0};
    }

    //the position in the list of the button under a point, -1 if there is none
    [[nodiscard]] int indexAt(float px, float py, std::size_t count) const noexcept {
        if (pitch <= 0.0f) {
            return -1;
        }
        for (std::size_t index = 0; index < count; ++index) {
            if (slot(index, count).contains(px, py)) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }
};

//the way the arrow keys move between the buttons
enum class Direction { Up, Down, Left, Right };

//the button next to this one in a direction, -1 if there is none.
//left and right walk along the row; up and down go to the row above or below keeping the column,
//and take the last button of that row when it is shorter
[[nodiscard]] inline int neighbour(const ButtonStack& stack, std::size_t index, std::size_t count,
                                   Direction direction) noexcept {
    if (index >= count) {
        return -1;
    }
    const ButtonStack::Place place = stack.placeOf(index, count);
    switch (direction) {
        case Direction::Left:
            return place.column > 0 ? static_cast<int>(index) - 1 : -1;
        case Direction::Right:
            return place.column + 1 < place.count ? static_cast<int>(index) + 1 : -1;
        case Direction::Up:
        case Direction::Down: {
            const int row = place.row + (direction == Direction::Down ? 1 : -1);
            if (row < 0) {
                return -1;
            }
            const ButtonStack::RowInfo info = stack.rowInfo(row, count);
            if (info.count <= 0) {
                return -1;
            }
            return info.first + std::min(place.column, info.count - 1);
        }
    }
    return -1;
}

}  //namespace ljn
