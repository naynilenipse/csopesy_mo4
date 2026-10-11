#pragma once //desktop header file, draws the desktop and handles the events

#include "imgui.h"
#include "core/Config.h"
#include <string>
#include <functional>
#include <utility>

class Desktop  {
    public:
        void Load (const Config& config); //reads the config and loads the desktop
        // renders the desktop every frame. The insets are how much of the top/bottom
        // edge the taskbar covers, so the clock and PWR button stay out from under it.
        void Render (float topInset = 0.0f, float bottomInset = 0.0f);

        // Application gives Desktop a function to call when PWR is clicked
        void SetOnShutdown (std::function<void()> cb) { onShutdown_ = std::move(cb); }

    private:
        void  DrawWallpaper (ImDrawList* draw, ImVec2 screen); //draws the wallpaper
        void DrawClock (ImDrawList* draw, ImVec2 screen); //draws the clock
        void DrawPowerButton (); //draws the PWR button

        ImVec4 gradientTop_ = ImVec4(0.0f, 0.35f, 0.65f, 1.0f); //the first gradient color
        ImVec4 gradientBottom_ = ImVec4(0.02f, 0.08f, 0.2f, 1.0f); //the second gradient color

        std::string wallpaperMode_ = "gradient";    // gradient, solid, or pattern
        std::string clockCorner_   = "top_right";   // top_left, top_right, bottom_left, bottom_right
        bool        clock24h_      = false;         // true = 24-hour clock

        std::function<void()> onShutdown_;          // called when PWR is clicked

        float topInset_    = 0.0f;                  // space taken by the taskbar at the top
        float bottomInset_ = 0.0f;                  // space taken by the taskbar at the bottom
};