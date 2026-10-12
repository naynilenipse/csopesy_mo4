#pragma once
// ---------------------------------------------------------------------------
// Placeholder2: a mock video player window. Taskbar icon is the smiley face.
// The window shows an image (assets/video_frame.png) as the "video" and a row of
// clickable buttons. Nothing plays and there is no sound; clicking a button just
// updates the "Last action" text.
// ---------------------------------------------------------------------------
#include "apps/App.h"
#include "core/Texture.h"

class Placeholder2 : public App {
public:
    Placeholder2();

    // Taskbar icon: assets/smiley.png on a colored tile (shapes if the PNG is missing).
    void DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const override;

protected:
    void RenderContents() override;

private:
    // Loads the PNGs on first use. OpenGL isn't ready when the constructor runs.
    void EnsureTextures() const;

    mutable Texture icon_;         // assets/smiley.png
    mutable Texture frame_;        // assets/video_frame.png  <- put your own image here
    mutable bool    texturesTried_ = false;

    // Only used to change button labels and the status line (no real playback)
    bool        playing_    = false;
    bool        muted_      = false;
    const char* lastAction_ = "None";
};