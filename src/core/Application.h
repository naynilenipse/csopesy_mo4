#pragma once
// ---------------------------------------------------------------------------
// Application: the "engine" of our fake OS.
//
// It does three jobs:
//   1. Init()     - opens the window, sets up OpenGL and ImGui
//   2. MainLoop() - every frame: read input, draw everything, show it
//   3. Shutdown() - cleans everything up in reverse order
//
// It also owns the OS "state machine": which screen are we on right now?
//   BOOT -> DESKTOP -> SHUTTING_DOWN -> (program ends)
// ---------------------------------------------------------------------------
#include <vector>
#include "core/Config.h"
#include "shell/Desktop.h"
#include "shell/Taskbar.h"
#include "apps/PlaceholderApp.h"
#include "apps/Placeholder2.h"
#include "apps/TaskManager.h"

struct GLFWwindow;   // "there is a type called GLFWwindow" - full details live in GLFW's header

enum class AppState {
    Boot,          // BIOS / splash screen (stretch goal, not built yet)
    Desktop,       // normal desktop: wallpaper, taskbar, app windows
    ShuttingDown,  // "Shutting down..." screen, then the program ends
};

class Application {
public:
    // Runs the whole program. Returns 0 on a clean shutdown (what main() returns).
    int Run();

    // Called by the PWR button. Starts the shutdown sequence.
    // This is the ONLY way the program is allowed to close.
    void RequestShutdown();

    const Config& GetConfig() const { return config_; }

private:
    bool Init();
    void MainLoop();
    void Shutdown();

    void RenderFrame(float deltaSeconds);   // decides what to draw based on state_
    void RenderShutdownScreen();
    void RenderCloseBlockedToast(float deltaSeconds);
    void RenderDebugPanel();                // TEMPORARY - delete once the real desktop exists

    // GLFW calls this when the user clicks the window's X button.
    static void OnWindowCloseRequested(GLFWwindow* window);

    Config      config_;
    Desktop    desktop_;
    Taskbar    taskbar_;

    // The apps on the taskbar. Buttons 1 and 2 are placeholders until their
    // real screens exist; button 3 is the TaskManager.
    PlaceholderApp computerApp_{ "My Computer", IM_COL32(0, 120, 215, 255), 1.2f, 38.4f,
                                 { "OS: CSOPESY Desktop OS", "Version: 1.0 (MO4 build)",
                                   "Processor: CSOPESY Virtual CPU @ 3.20GHz", "Memory: 16.0 GB" } };
    PlaceholderApp notesApp_   { "Notes", IM_COL32(200, 140, 0, 255), 0.4f, 22.1f,
                                 { "Welcome to Notes!", "Your notes will appear here." } };
    Placeholder2   placeholder2_;
    TaskManager    taskManager_;
    std::vector<App*> apps_;   // taskbar order, left to right

    GLFWwindow* window_ = nullptr;
    AppState    state_  = AppState::Desktop;
    bool        running_ = true;

    float shutdownTimer_   = 0.0f;   // seconds spent on the shutdown screen
    float closeToastTimer_ = 0.0f;   // seconds left to show "use PWR" message
};