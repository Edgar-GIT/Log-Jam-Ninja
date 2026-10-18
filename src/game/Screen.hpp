// Screen.hpp - the game window and the fixed 960x540 picture inside it, scaled to the window without blur.

#pragma once

#include <raylib.h>

namespace ljn {

//opens the window when created and closes it when destroyed
//the game draws into a small texture (960x540) that is scaled up to fill the window
class Screen {
public:
    //the size of the picture the game draws, in pixels
    static constexpr int kWidth = 960;
    static constexpr int kHeight = 540;

    explicit Screen(const char* title);
    ~Screen();

    //there is only one window, so it cannot be copied
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    //everything drawn between these two calls goes into the 960x540 picture
    void beginPicture();
    void endPicture();

    //puts the picture on the window, centered, with black bars where it does not fill
    void present();

    //F11 style switches
    void toggleFullscreen();
    void toggleIntegerScale() noexcept { m_integerScale = !m_integerScale; }

    //how many times the picture is enlarged right now (can be fractional when integer scale is off)
    [[nodiscard]] float scale() const noexcept;

    //where the picture is on the window, in window pixels
    [[nodiscard]] Rectangle destination() const noexcept;

    //converts a position on the window (the mouse) into a position in the picture
    [[nodiscard]] Vector2 toPicture(Vector2 windowPosition) const noexcept;

private:
    RenderTexture2D m_picture{};

    //on: only whole multiples (2x, 3x), pixels stay perfect. off: any size that fits, pixels can be uneven
    bool m_integerScale{true};
};

}  //namespace ljn
