#pragma once

#include <cstdint>
#include "imgui.h"

// File for loading PNGs
// A PNG/JPG loaded into an OpenGL texture so ImGui can draw it.
// IMPORTANT: Load() needs a live OpenGL context, so only call it AFTER
// glfwMakeContextCurrent() (i.e. after Application::Init has opened the window).
struct Texture {
    unsigned int id = 0;   // OpenGL texture handle (0 = not loaded)
    int width = 0;
    int height = 0;

    // Tries "path" first, then "../path" (same fallback Application uses for the
    // config, so it works when started from inside build/).
    bool Load(const char* path);
    void Free();

    bool IsLoaded() const { return id != 0; }
    ImTextureID ImId() const { return (ImTextureID)(intptr_t)id; }
    ImVec2 Size() const { return ImVec2((float)width, (float)height); }
};