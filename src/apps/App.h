#pragma once
// ---------------------------------------------------------------------------
// App: the base class for every window that can be opened from the taskbar.
//
// Like on Windows, an app is always in one of three states:
//   closed    - not running: not drawn, no taskbar indicator, not in Task Manager
//   open      - running and drawn as an ImGui window
//   minimized - still running (taskbar indicator + Task Manager row), but not drawn
//
// HOW TO MAKE A NEW APP:
//   1. Inherit from App and pass a name, a default window size and a color.
//   2. Fill in RenderContents() with what goes INSIDE the window.
//      App already draws the window itself (title bar, X button, focus).
//   3. (Optional) Override DrawIcon() for a custom taskbar icon.
//   4. Add it to Application's apps_ list so the taskbar shows a button for it.
// ---------------------------------------------------------------------------
#include "imgui.h"

class App {
public:
    App(const char* name, ImVec2 defaultSize, ImU32 accentColor);
    virtual ~App() = default;

    const char* GetName() const { return name_; }

    bool IsRunning()   const { return running_; }
    bool IsMinimized() const { return minimized_; }
    bool IsVisible()   const { return running_ && !minimized_; }
    bool IsFocused()   const { return focused_; }   // true if it was the active window this frame

    void Launch();     // start it, or restore it and bring it to the front if it's already running
    void Minimize();   // hide it but keep it running
    void Close();      // stop it (same as clicking the window's X)

    // Called every frame. Draws nothing unless the app is open.
    void Render();

    // Taskbar icon, drawn inside the box [min, max]. Default: colored tile with the first letter.
    virtual void DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const;

    // Dummy usage numbers that Task Manager shows for this app.
    float GetBaseCpu()      const { return baseCpu_; }
    float GetBaseMemoryMB() const { return baseMemoryMB_; }

protected:
    virtual void RenderContents() = 0;   // what's inside the window

    ImU32 accent_;
    float baseCpu_      = 0.5f;    // percent
    float baseMemoryMB_ = 25.0f;

private:
    const char* name_;
    ImVec2      defaultSize_;
    ImVec2      cascadeOffset_;   // so new windows don't all open in exactly the same spot

    bool running_        = false;
    bool minimized_      = false;
    bool focused_        = false;
    bool focusRequested_ = false;   // bring to the front on the next frame
};
