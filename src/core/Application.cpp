#include "core/Application.h"

#include <cstdio>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ===========================================================================
//  Run: the whole life of the program
// ===========================================================================
int Application::Run() {
    if (!Init()) return 1;   // 1 = something went wrong while starting
    MainLoop();
    Shutdown();
    return 0;                // 0 = clean exit (via PWR)
}

// ===========================================================================
//  Init: open window + set up ImGui
// ===========================================================================
bool Application::Init() {
    // Look for the config in the project folder. The second path is a backup
    // in case the program is started from inside the build/ folder.
    if (!config_.Load("config/config.txt"))
        config_.Load("../config/config.txt");

    if (!glfwInit()) {
        std::printf("[App] Failed to start GLFW.\n");
        return false;
    }

    // Ask for an OpenGL 3.0 context (works on basically every Windows PC).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    std::string title = config_.GetString("window_title", "CSOPESY Desktop OS Emulator");
    int width  = config_.GetInt("window_width", 1280);
    int height = config_.GetInt("window_height", 720);

    // fullscreen = true -> borderless window covering the whole monitor
    GLFWmonitor* monitor = nullptr;
    if (config_.GetBool("fullscreen", false)) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width  = mode->width;
        height = mode->height;
    }

    window_ = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!window_) {
        std::printf("[App] Failed to create window.\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);   // vsync: draw at the monitor's refresh rate, saves CPU

    // Block the X button: GLFW will call our function instead of closing.
    // The "user pointer" lets that static function find this Application object.
    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowCloseCallback(window_, OnWindowCloseRequested);

    // ---- ImGui setup (standard boilerplate from the ImGui examples) ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // don't save window positions to imgui.ini (keeps runs identical)
    io.FontGlobalScale = config_.GetFloat("ui_scale", 1.0f);   // bigger text for the video

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Let the Desktop's PWR button trigger our shutdown sequence
    desktop_.SetOnShutdown([this]() { RequestShutdown(); });
    desktop_.Load(config_);

    // Taskbar buttons, left to right. Task Manager lists the same apps under "Apps".
    apps_ = { &computerApp_, &notesApp_, &taskManager_ };
    taskManager_.Load(config_);
    taskManager_.SetApps(apps_);
    taskbar_.Load(config_);
    taskbar_.SetApps(apps_);

    std::printf("[App] Started (%dx%d).\n", width, height);
    return true;
}

// ===========================================================================
//  MainLoop: runs once per frame (~60 times a second) until shutdown finishes
// ===========================================================================
void Application::MainLoop() {
    while (running_) {
        glfwPollEvents();   // read mouse/keyboard/window events

        // Start a new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderFrame(ImGui::GetIO().DeltaTime);

        // Turn everything we described into pixels
        ImGui::Render();
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);   // show the finished frame
    }
}

// ===========================================================================
//  Shutdown: undo Init in reverse order
// ===========================================================================
void Application::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
    std::printf("[App] Clean shutdown complete.\n");
}

// ===========================================================================
//  RenderFrame: what to draw depends on which "screen" the OS is on
// ===========================================================================
void Application::RenderFrame(float deltaSeconds) {
    switch (state_) {
        case AppState::Boot:
            // TODO (stretch): BootScreen - BIOS text, then splash, then go to Desktop
            state_ = AppState::Desktop;
            break;

        case AppState::Desktop:
            // Order matters: wallpaper first, then app windows, then the taskbar
            // (the taskbar needs to know which app window was active this frame).
            desktop_.Render(taskbar_.GetTopInset(), taskbar_.GetBottomInset());
            for (App* app : apps_) app->Render();
            taskbar_.Render();
            RenderDebugPanel();
            break;

        case AppState::ShuttingDown:
            RenderShutdownScreen();
            shutdownTimer_ += deltaSeconds;
            if (shutdownTimer_ >= config_.GetFloat("shutdown_duration_sec", 2.0f))
                running_ = false;   // ends MainLoop -> Shutdown() -> program exits cleanly
            break;
    }

    RenderCloseBlockedToast(deltaSeconds);
}

void Application::RequestShutdown() {
    if (state_ == AppState::ShuttingDown) return;   // already shutting down
    std::printf("[App] Shutdown requested via PWR.\n");
    state_ = AppState::ShuttingDown;
    shutdownTimer_ = 0.0f;
}

// Called by GLFW when the X button (or Alt+F4) is pressed.
void Application::OnWindowCloseRequested(GLFWwindow* window) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glfwSetWindowShouldClose(window, GLFW_FALSE);   // cancel the close
    app->closeToastTimer_ = 3.0f;                    // show the hint for 3 seconds
}

// ===========================================================================
//  Small screens / overlays
// ===========================================================================
void Application::RenderShutdownScreen() {
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImVec2 size = ImGui::GetIO().DisplaySize;
    draw->AddRectFilled(ImVec2(0, 0), size, IM_COL32(0, 0, 0, 255));

    const char* text = "Shutting down...";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    draw->AddText(ImVec2((size.x - textSize.x) * 0.5f, (size.y - textSize.y) * 0.5f),
                  IM_COL32(255, 255, 255, 255), text);
}

void Application::RenderCloseBlockedToast(float deltaSeconds) {
    if (closeToastTimer_ <= 0.0f) return;
    closeToastTimer_ -= deltaSeconds;

    const char* text = "Please use the PWR button to shut down.";
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 size = ImGui::GetIO().DisplaySize;
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 pos((size.x - textSize.x) * 0.5f, 40.0f);
    draw->AddRectFilled(ImVec2(pos.x - 12, pos.y - 8),
                        ImVec2(pos.x + textSize.x + 12, pos.y + textSize.y + 8),
                        IM_COL32(180, 40, 40, 230), 6.0f);
    draw->AddText(pos, IM_COL32(255, 255, 255, 255), text);
}

// TEMPORARY: proves the skeleton works. Replace with the real Desktop later.
void Application::RenderDebugPanel() {
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::Begin("Skeleton Check (temporary)");
    ImGui::Text("Window, OpenGL and ImGui are working.");
    ImGui::Text("Config file: %s", config_.GetPath().c_str());
    ImGui::Text("taskbar_position = %s", config_.GetString("taskbar_position", "bottom").c_str());
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::TextWrapped("Try the window's X button - it should refuse to close.");
    ImGui::End();
}