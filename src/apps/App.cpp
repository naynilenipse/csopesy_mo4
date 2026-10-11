#include "apps/App.h"

// Counts how many apps have been created, so each one gets its own cascade offset.
static int s_appCount = 0;

App::App(const char* name, ImVec2 defaultSize, ImU32 accentColor)
    : accent_(accentColor), name_(name), defaultSize_(defaultSize) {
    float step = 30.0f * (float)(s_appCount % 5) - 60.0f;   // -60, -30, 0, 30, 60
    cascadeOffset_ = ImVec2(step, step);
    s_appCount++;
}

void App::Launch() {
    running_   = true;
    minimized_ = false;
    focusRequested_ = true;
}

void App::Minimize() {
    if (running_) minimized_ = true;
}

void App::Close() {
    running_   = false;
    minimized_ = false;
}

// ===========================================================================
//  Render: draws the window frame, then asks the subclass to fill it in
// ===========================================================================
void App::Render() {
    focused_ = false;              // hidden windows are never the active one
    if (!IsVisible()) return;      // immediate mode: not calling Begin() = not on screen

    // First time only: open near the middle of the screen (later, ImGui remembers
    // where the user dragged it, even after minimizing or closing).
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImVec2 pos(screen.x * 0.5f + cascadeOffset_.x, screen.y * 0.5f + cascadeOffset_.y);
    ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(defaultSize_, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(1.0f);   // solid, so windows behind don't show through

    if (focusRequested_) {
        ImGui::SetNextWindowFocus();
        focusRequested_ = false;
    }

    bool keepOpen = true;   // ImGui sets this to false when the X is clicked
    if (ImGui::Begin(name_, &keepOpen, ImGuiWindowFlags_NoCollapse)) {
        focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        RenderContents();
    }
    ImGui::End();

    if (!keepOpen) Close();
}

// Default icon: a rounded tile in the app's color with its first letter in the middle.
void App::DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const {
    draw->AddRectFilled(min, max, accent_, 6.0f);

    char letter[2] = { name_[0], '\0' };
    ImVec2 textSize = ImGui::CalcTextSize(letter);
    ImVec2 c((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    draw->AddText(ImVec2(c.x - textSize.x * 0.5f, c.y - textSize.y * 0.5f),
                  IM_COL32(255, 255, 255, 255), letter);
}
