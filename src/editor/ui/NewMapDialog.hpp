// NewMapDialog.hpp - the window that asks for the size of a new map.

#pragma once

#include <optional>
#include <raylib.h>

namespace ljn {

//size of a map in cells
struct MapSize {
    int width{0};
    int height{0};
};

class NewMapDialog {
public:
    //opens the window, it remembers the size of the last time
    void open() noexcept { m_open = true; }

    [[nodiscard]] bool isOpen() const noexcept { return m_open; }

    //draws the window, returns the size when "Create" was pressed
    [[nodiscard]] std::optional<MapSize> draw(Vector2 screenSize);

private:
    bool m_open{false};

    //the numbers in the two boxes, and whether the user is typing in them
    int m_width{120};
    int m_height{30};
    bool m_editWidth{false};
    bool m_editHeight{false};
};

}  //namespace ljn
