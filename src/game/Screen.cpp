// Screen.cpp - implementation of the window and the scaled picture.

#include "Screen.hpp"

#include <algorithm>
#include <cmath>

namespace ljn {

Screen::Screen(const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(kWidth, kHeight, title);
    SetWindowMinSize(kWidth, kHeight);
    SetTargetFPS(60);

    //start as big as the monitor allows with a whole multiple of the picture (the reference is 1080p, so 2x)
    const int monitor = GetCurrentMonitor();
    const int fitX = GetMonitorWidth(monitor) / kWidth;
    const int fitY = (GetMonitorHeight(monitor) * 9 / 10) / kHeight;
    const int multiple = std::clamp(std::min(fitX, fitY), 1, 4);
    SetWindowSize(kWidth * multiple, kHeight * multiple);
    SetWindowPosition((GetMonitorWidth(monitor) - kWidth * multiple) / 2,
                      (GetMonitorHeight(monitor) - kHeight * multiple) / 2);

    //the picture: nearest filter keeps every pixel sharp when it is enlarged
    m_picture = LoadRenderTexture(kWidth, kHeight);
    SetTextureFilter(m_picture.texture, TEXTURE_FILTER_POINT);
}

Screen::~Screen() {
    UnloadRenderTexture(m_picture);
    CloseWindow();
}

void Screen::beginPicture() {
    BeginTextureMode(m_picture);
    ClearBackground(BLACK);
}

void Screen::endPicture() {
    EndTextureMode();
}

float Screen::scale() const noexcept {
    const float fit = std::min(static_cast<float>(GetScreenWidth()) / static_cast<float>(kWidth),
                               static_cast<float>(GetScreenHeight()) / static_cast<float>(kHeight));

    //whole multiples only, but never below 1 (a window smaller than the picture is not allowed anyway)
    return m_integerScale ? std::max(1.0f, std::floor(fit)) : fit;
}

Rectangle Screen::destination() const noexcept {
    const float size = scale();
    const float width = static_cast<float>(kWidth) * size;
    const float height = static_cast<float>(kHeight) * size;

    //centered, and rounded to whole pixels so the edges stay sharp
    return {std::round((static_cast<float>(GetScreenWidth()) - width) * 0.5f),
            std::round((static_cast<float>(GetScreenHeight()) - height) * 0.5f), width, height};
}

Vector2 Screen::toPicture(Vector2 windowPosition) const noexcept {
    const Rectangle area = destination();
    const float size = scale();
    return {(windowPosition.x - area.x) / size, (windowPosition.y - area.y) / size};
}

void Screen::present() {
    BeginDrawing();
    ClearBackground(BLACK);

    //render textures are upside down, so the source height is negative
    const Rectangle source{0.0f, 0.0f, static_cast<float>(kWidth), -static_cast<float>(kHeight)};
    DrawTexturePro(m_picture.texture, source, destination(), {0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

void Screen::toggleFullscreen() {
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
        return;
    }

    //fullscreen uses the size of the monitor
    const int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen();
}

}  //namespace ljn
