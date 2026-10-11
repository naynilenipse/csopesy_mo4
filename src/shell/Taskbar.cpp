#include "shell/Taskbar.h"

#include "imgui_internal.h"   // for BringWindowToDisplayFront (keeps the taskbar above app windows)

void Taskbar::Load(const Config& config) {
    atTop_    = config.GetString("taskbar_position", "bottom") == "top";
    centered_ = config.GetString("taskbar_alignment", "center") != "left";
    height_   = config.GetFloat("taskbar_height", height_);
    if (height_ < 32.0f) height_ = 32.0f;   // any smaller and the icons become unclickable
}

// ===========================================================================
//  Render: the bar itself, then one button per app
// ===========================================================================
void Taskbar::Render() {
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    float y = atTop_ ? 0.0f : screen.y - height_;

    ImGui::SetNextWindowPos(ImVec2(0, y));
    ImGui::SetNextWindowSize(ImVec2(screen.x, height_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(18, 20, 28, 235));   // dark, slightly see-through

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoFocusOnAppearing;
    ImGui::Begin("##taskbar", nullptr, flags);

    // Clicking a window normally moves it to the top of the pile. Re-raising the
    // taskbar every frame keeps it above app windows that get dragged over it.
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

    UpdateActiveApp(ImGui::IsWindowFocused());

    // Thin line on the edge that faces the desktop, so the bar stands out
    ImDrawList* draw = ImGui::GetWindowDrawList();
    float edgeY = atTop_ ? y + height_ - 1.0f : y;
    draw->AddLine(ImVec2(0, edgeY), ImVec2(screen.x, edgeY), IM_COL32(255, 255, 255, 40));

    // Lay the buttons out in a row, either centered or from the left
    const float size = height_ - 8.0f;   // square buttons, 4 px gap above and below
    const float gap  = 6.0f;
    float rowWidth = apps_.size() * size + (apps_.size() > 0 ? (apps_.size() - 1) * gap : 0.0f);
    float x = centered_ ? (screen.x - rowWidth) * 0.5f : 8.0f;

    for (size_t i = 0; i < apps_.size(); i++) {
        ImGui::SetCursorScreenPos(ImVec2(x + i * (size + gap), y + 4.0f));
        AppButton(apps_[i], size);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

// Works out which app counts as "active".
// Problem: pressing a taskbar button moves focus to the taskbar itself, so by the
// time the click finishes, no app is focused anymore. So we remember the last
// focused app, and keep remembering it while the taskbar has focus.
void Taskbar::UpdateActiveApp(bool taskbarFocused) {
    App* focused = nullptr;
    for (App* app : apps_)
        if (app->IsFocused()) focused = app;

    if (focused)              activeApp_ = focused;
    else if (!taskbarFocused) activeApp_ = nullptr;   // user clicked the desktop, nothing is active

    if (activeApp_ && !activeApp_->IsVisible()) activeApp_ = nullptr;   // it was closed/minimized
}

void Taskbar::AppButton(App* app, float size) {
    ImGui::PushID(app);
    bool clicked = ImGui::InvisibleButton("##app", ImVec2(size, size));
    bool hovered = ImGui::IsItemHovered();
    bool held    = ImGui::IsItemActive();
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImGui::PopID();

    bool active = (app == activeApp_);

    if (clicked) {
        if (active) {
            app->Minimize();
            activeApp_ = nullptr;
        } else {
            app->Launch();   // opens, restores, or brings to front - whichever is needed
            activeApp_ = app;
        }
    }

    if (hovered) ImGui::SetTooltip("%s", app->GetName());

    // Button background: brighter when hovered/pressed/active
    ImDrawList* draw = ImGui::GetWindowDrawList();
    int alpha = held ? 50 : hovered ? 35 : active ? 22 : 0;
    if (alpha > 0) draw->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, alpha), 6.0f);

    // Icon in the middle, with some padding
    float pad = size * 0.2f;
    app->DrawIcon(draw, ImVec2(p0.x + pad, p0.y + pad), ImVec2(p1.x - pad, p1.y - pad));

    // Running indicator along the bottom of the button
    if (app->IsRunning()) {
        float w = active ? size * 0.4f : size * 0.15f;
        float cx = (p0.x + p1.x) * 0.5f;
        ImU32 color = active ? IM_COL32(96, 205, 255, 255) : IM_COL32(170, 170, 180, 255);
        draw->AddRectFilled(ImVec2(cx - w * 0.5f, p1.y - 3.0f), ImVec2(cx + w * 0.5f, p1.y),
                            color, 1.5f);
    }
}
