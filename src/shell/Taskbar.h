#pragma once
// ---------------------------------------------------------------------------
// Taskbar: the fixed panel along the top or bottom edge of the screen.
//
// It shows one icon button per app. Clicking works like on Windows:
//   app closed                     -> open it
//   app open but behind something  -> bring it to the front
//   app is the active window       -> minimize it
//   app minimized                  -> restore it
// A line under each icon shows which apps are running (long + blue = active).
//
// Settings (config.txt): taskbar_position, taskbar_alignment, taskbar_height
// ---------------------------------------------------------------------------
#include <vector>
#include "imgui.h"
#include "core/Config.h"
#include "apps/App.h"

class Taskbar {
public:
    void Load(const Config& config);
    void SetApps(const std::vector<App*>& apps) { apps_ = apps; }
    void Render();   // call AFTER the apps render, so it knows which app is active

    // How much of the screen edge the taskbar covers, so the Desktop can keep
    // the clock and PWR button out from under it.
    float GetTopInset()    const { return atTop_ ? height_ : 0.0f; }
    float GetBottomInset() const { return atTop_ ? 0.0f : height_; }

private:
    void UpdateActiveApp(bool taskbarFocused);
    void AppButton(App* app, float size);

    std::vector<App*> apps_;                  // not owned: Application owns the apps
    App*              activeApp_ = nullptr;   // the app whose window was in front before the taskbar was clicked

    bool  atTop_    = false;   // taskbar_position = top / bottom
    bool  centered_ = true;    // taskbar_alignment = center / left
    float height_   = 48.0f;
};
