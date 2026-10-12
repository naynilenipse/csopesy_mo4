#include "core/Application.h"

#include <cstdio>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/Texture.h"
#include "apps/Placeholder2.h"

// ===========================================================================
//  DrawBackgroundImage: draws assets/background.jpg behind everything,
//  scaled to fill the screen (keeps the aspect ratio, crops the overflow).
//  Does nothing if the image can't be loaded, so the desktop gradient shows.
// ===========================================================================
static void DrawBackgroundImage() {
    static Texture bg;
    static bool tried = false;
    if (!tried) {
        tried = true;
        bg.Load("assets/background.jpg");   // needs the GL context, so load on first draw
        std::printf("[BG] loaded=%d  size=%dx%d\n",
                    (int)bg.IsLoaded(), bg.width, bg.height);
    }
    if (!bg.IsLoaded()) return;

    ImVec2 screen = ImGui::GetIO().DisplaySize;
    float screenAspect = screen.x / screen.y;
    float imageAspect  = (float)bg.width / (float)bg.height;
    ImVec2 uv0(0, 0), uv1(1, 1);
    if (imageAspect > screenAspect) {              // image is wider: crop left/right
        float visible = screenAspect / imageAspect;
        uv0.x = (1.0f - visible) * 0.5f;
        uv1.x = uv0.x + visible;
    } else {                                       // image is taller: crop top/bottom
        float visible = imageAspect / screenAspect;
        uv0.y = (1.0f - visible) * 0.5f;
        uv1.y = uv0.y + visible;
    }
    ImGui::GetBackgroundDrawList()->AddImage(bg.ImId(), ImVec2(0, 0), screen, uv0, uv1);
}

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

    // Block the X button: GLFW calls our function instead of closing.
    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowCloseCallback(window_, OnWindowCloseRequested);

    // ---- ImGui setup ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // don't save window positions to imgui.ini
    io.FontGlobalScale = config_.GetFloat("ui_scale", 1.0f);

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Let the Desktop's PWR button trigger our shutdown sequence
    desktop_.SetOnShutdown([this]() { RequestShutdown(); });
    desktop_.Load(config_);

    // Taskbar buttons, left to right. Task Manager lists the same apps under "Apps".
    apps_ = { &computerApp_, &notesApp_, &taskManager_, &placeholder2_ };
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
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderFrame(ImGui::GetIO().DeltaTime);

        ImGui::Render();
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
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
            state_ = AppState::Desktop;
            break;

        case AppState::Desktop:
            // Wallpaper FIRST, then the desktop (clock, power button), then
            // the app windows, then the taskbar. They share the background
            // draw list, so order == z-order.
            DrawBackgroundImage();
            desktop_.Render(taskbar_.GetTopInset(), taskbar_.GetBottomInset());
            for (App* app : apps_) app->Render();
            taskbar_.Render();
            break;

        case AppState::ShuttingDown:
            RenderShutdownScreen();
            shutdownTimer_ += deltaSeconds;
            if (shutdownTimer_ >= config_.GetFloat("shutdown_duration_sec", 2.0f))
                running_ = false;
            break;
    }

    RenderCloseBlockedToast(deltaSeconds);
}

void Application::RequestShutdown() {
    if (state_ == AppState::ShuttingDown) return;
    std::printf("[App] Shutdown requested via PWR.\n");
    state_ = AppState::ShuttingDown;
    shutdownTimer_ = 0.0f;
}

void Application::OnWindowCloseRequested(GLFWwindow* window) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    app->closeToastTimer_ = 3.0f;
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