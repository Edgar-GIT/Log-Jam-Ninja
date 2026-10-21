// Input.cpp - implementation of the input, it reads the raylib keys and the mouse.

#include "Input.hpp"

#include <cstddef>
#include "input/Keys.hpp"

namespace ljn {
namespace {

//the core does not know raylib, so the key codes have to be the same on both sides
static_assert(key::Space == KEY_SPACE, "the key codes of the core and of raylib must be the same");
static_assert(key::A == KEY_A, "the key codes of the core and of raylib must be the same");
static_assert(key::Z == KEY_Z, "the key codes of the core and of raylib must be the same");
static_assert(key::Escape == KEY_ESCAPE, "the key codes of the core and of raylib must be the same");
static_assert(key::Enter == KEY_ENTER, "the key codes of the core and of raylib must be the same");
static_assert(key::Tab == KEY_TAB, "the key codes of the core and of raylib must be the same");
static_assert(key::Backspace == KEY_BACKSPACE, "the key codes of the core and of raylib must be the same");
static_assert(key::Delete == KEY_DELETE, "the key codes of the core and of raylib must be the same");
static_assert(key::Right == KEY_RIGHT, "the key codes of the core and of raylib must be the same");
static_assert(key::Left == KEY_LEFT, "the key codes of the core and of raylib must be the same");
static_assert(key::Down == KEY_DOWN, "the key codes of the core and of raylib must be the same");
static_assert(key::Up == KEY_UP, "the key codes of the core and of raylib must be the same");
static_assert(key::F1 == KEY_F1, "the key codes of the core and of raylib must be the same");
static_assert(key::F12 == KEY_F12, "the key codes of the core and of raylib must be the same");

//the position of an action in the arrays of the class
constexpr std::size_t index(Action action) noexcept {
    return static_cast<std::size_t>(action);
}

//the last key code of raylib, used to look for any key that was pressed
constexpr int kLastKey = KEY_MENU;

}  //anonymous namespace

void Input::update(const KeyBindings& bindings) {
    for (const Action action : kAllActions) {
        bool down = false;
        bool pressed = false;

        //an action has two keys, the main one and the alternative one
        for (int slot = 0; slot < kSlotsPerAction; ++slot) {
            const int code = bindings.key(action, slot);
            if (code == key::None) {
                continue;
            }
            down = down || IsKeyDown(code);
            pressed = pressed || IsKeyPressed(code);
        }

        m_down[index(action)] = down;
        m_pressed[index(action)] = pressed;
    }

    //the mouse: it is only interesting when it moves or when a button is pressed
    const Vector2 mouse = GetMousePosition();
    m_mouseMoved = mouse.x != m_mouse.x || mouse.y != m_mouse.y;
    m_mouse = mouse;
}

bool Input::down(Action action) const noexcept {
    return m_down[index(action)];
}

bool Input::pressed(Action action) const noexcept {
    return m_pressed[index(action)];
}

float Input::moveAxis() const noexcept {
    const float right = down(Action::Right) ? 1.0f : 0.0f;
    const float left = down(Action::Left) ? 1.0f : 0.0f;
    return right - left;
}

bool Input::menuUp() const noexcept {
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
}

bool Input::menuDown() const noexcept {
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
}

bool Input::menuLeft() const noexcept {
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
}

bool Input::menuRight() const noexcept {
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
}

bool Input::menuConfirm() const noexcept {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

bool Input::menuBack() const noexcept {
    return IsKeyPressed(KEY_ESCAPE);
}

int Input::anyKeyPressed() const noexcept {
    for (int code = 0; code <= kLastKey; ++code) {
        if (IsKeyPressed(code)) {
            return code;
        }
    }
    return key::None;
}

bool Input::keyPressed(int key) const noexcept {
    return IsKeyPressed(key);
}

bool Input::keyDown(int key) const noexcept {
    return IsKeyDown(key);
}

bool Input::mousePressed() const noexcept {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool Input::mouseDown() const noexcept {
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

Vector2 Input::mousePosition() const noexcept {
    return m_mouse;
}

}  //namespace ljn
