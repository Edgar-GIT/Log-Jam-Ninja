// Input.hpp - turns the keys into actions, using the bindings of the settings.

#pragma once

#include <raylib.h>
#include "input/Action.hpp"
#include "input/KeyBindings.hpp"

namespace ljn {

//reads the keyboard and the mouse once per frame and remembers what is held down and what was pressed
class Input {
public:
    //reads the keyboard, the bindings say which key does what
    void update(const KeyBindings& bindings);

    //true while the key of the action is held down
    [[nodiscard]] bool down(Action action) const noexcept;

    //true in the frame the key of the action went down
    [[nodiscard]] bool pressed(Action action) const noexcept;

    //-1 left, 0 nothing, 1 right
    [[nodiscard]] float moveAxis() const noexcept;

    //the menus use fixed keys, they never change
    [[nodiscard]] bool menuUp() const noexcept;
    [[nodiscard]] bool menuDown() const noexcept;
    [[nodiscard]] bool menuLeft() const noexcept;
    [[nodiscard]] bool menuRight() const noexcept;
    [[nodiscard]] bool menuConfirm() const noexcept;
    [[nodiscard]] bool menuBack() const noexcept;

    //any key that went down this frame, 0 if none (used to choose a new key)
    [[nodiscard]] int anyKeyPressed() const noexcept;

    //the keys of the window (F11 and the like) are not actions
    [[nodiscard]] bool keyPressed(int key) const noexcept;
    [[nodiscard]] bool keyDown(int key) const noexcept;

    //the mouse
    [[nodiscard]] bool mousePressed() const noexcept;
    [[nodiscard]] bool mouseDown() const noexcept;
    [[nodiscard]] Vector2 mousePosition() const noexcept;

    //true if the mouse moved since the last frame
    [[nodiscard]] bool mouseMoved() const noexcept { return m_mouseMoved; }

private:
    //one flag for each action
    bool m_down[kActionCount]{};
    bool m_pressed[kActionCount]{};

    Vector2 m_mouse{0.0f, 0.0f};
    bool m_mouseMoved{false};
};

}  //namespace ljn
