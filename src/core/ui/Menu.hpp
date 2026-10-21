// Menu.hpp - the logic of a menu: a list of items, a cursor, and what happens when a key or the mouse is used.
// it knows nothing about drawing, so the screen code can show it with rectangles now and sprites later.

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ljn {

enum class ItemKind {
    //a title in the list, the cursor skips it
    Label,

    //does something when confirmed
    Button,

    //on or off
    Toggle,

    //a number between min and max
    Slider,

    //one of several texts
    Choice,
};

struct MenuItem {
    ItemKind kind{ItemKind::Button};

    //tells the screen code which item an event is about, it does not have to be the position
    int id{0};
    std::string label{};

    //text on the right side: a key name, a time, "Locked"...
    std::string detail{};

    //a disabled item can be selected but does nothing
    bool enabled{true};

    //toggle: 0 or 1. slider: the number. choice: the position in choices
    int value{0};
    int min{0};
    int max{1};
    int step{1};
    std::vector<std::string> choices{};
};

//what the player did in this frame, each field is true only in the frame of the key press
struct MenuInput {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool confirm{false};
    bool back{false};
};

enum class EventKind {
    None,

    //a button was confirmed
    Activated,

    //a toggle, slider or choice has a new value
    Changed,

    //the back key was pressed
    Back,
};

struct MenuEvent {
    EventKind kind{EventKind::None};

    //the id of the item (-1 for none and for back)
    int id{-1};

    [[nodiscard]] bool operator==(const MenuEvent&) const noexcept = default;
};

class Menu {
public:
    //builders for the common items
    static MenuItem label(std::string text);
    static MenuItem button(int id, std::string text);
    static MenuItem toggle(int id, std::string text, bool on);
    static MenuItem slider(int id, std::string text, int value, int min, int max, int step = 1);
    static MenuItem choice(int id, std::string text, std::vector<std::string> choices, int selected);

    //adds an item at the end, the cursor goes to the first item it can stand on
    void add(MenuItem item);
    void clear() noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return m_items.size(); }
    [[nodiscard]] const std::vector<MenuItem>& items() const noexcept { return m_items; }

    //the item under the cursor, null for an empty menu
    [[nodiscard]] const MenuItem* selected() const noexcept;
    [[nodiscard]] int cursor() const noexcept { return m_cursor; }

    //moves the cursor to a position (labels cannot be selected, so it does nothing for them)
    void setCursor(int index) noexcept;

    //the keys: up and down move the cursor (and wrap around), left and right change values, confirm uses the item
    MenuEvent update(const MenuInput& input);

    //the mouse: moving over an item selects it, clicking uses it. for a slider the click position decides the value
    //fraction is where in the slider the click was, from 0 (left end) to 1 (right end)
    void hover(int index) noexcept { setCursor(index); }
    MenuEvent click(int index, float fraction = 0.0f);

    //changes an item from outside, by id (loading values, translating a label...)
    MenuItem* find(int id) noexcept;
    [[nodiscard]] const MenuItem* find(int id) const noexcept;

private:
    //moves the cursor by one step in a direction, skipping labels
    void move(int direction) noexcept;

    //confirm or click on the item under the cursor
    MenuEvent use();

    //left or right on the item under the cursor
    MenuEvent change(int direction);

    std::vector<MenuItem> m_items;
    int m_cursor{0};
};

}  //namespace ljn
