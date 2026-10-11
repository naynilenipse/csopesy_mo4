#pragma once //desktop header file, draws the desktop and handles the events

#include "imgui.h"
#include "core/Config.h"

class Desktop  {
    public:
        void Load (const Config& config); //reads the config and loads the desktop
        void Render (); //renders the desktop every frame

    private:
        void  DrawWallpaper (ImDrawList* draw, ImVec2 screen); //draws the wallpaper
        void DrawClock (ImDrawList* draw, ImVec2 screen); //draws the clock

        ImVec4 gradientTop_ = ImVec4(0.0f, 0.35f, 0.65f, 1.0f); //the first gradient color
        ImVec4 gradientBottom_ = ImVec4(0.02f, 0.08f, 0.2f, 1.0f); //the second gradient color
};