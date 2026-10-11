#include "shell/Desktop.h"
#include <ctime>

void Desktop::Load (const Config& config) {
    // Load the desktop configuration from the provided config object, like the wallpaper path, gradient colors, etc.
    gradientTop_ = config.GetColor("wallpaper_top", gradientTop_);
    gradientBottom_ = config.GetColor("wallpaper_bottom", gradientBottom_);

    wallpaperMode_ = config.GetString("wallpaper_mode", wallpaperMode_);
    clockCorner_   = config.GetString("clock_corner",   clockCorner_);
    clock24h_      = config.GetBool  ("clock_24h",      clock24h_);
}

void Desktop::Render (float topInset, float bottomInset) {
    topInset_    = topInset;
    bottomInset_ = bottomInset;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();// Get the draw list for the background layer
    ImVec2 screen = ImGui::GetIO().DisplaySize; // Get the size of the display

    // Draw the wallpaper
    DrawWallpaper(draw, screen);
    DrawClock(draw, screen);
    DrawPowerButton();
}

void Desktop::DrawWallpaper (ImDrawList* draw, ImVec2 screen) {
    //Drawing functiions want colors packed as ImU32, so we convert the gradient colors from ImVec4 to ImU32
    ImU32 top = ImGui::ColorConvertFloat4ToU32(gradientTop_);
    ImU32 bottom = ImGui::ColorConvertFloat4ToU32(gradientBottom_);

    if (wallpaperMode_ == "solid") {
        // One flat color: the "top" color fills everything
        draw->AddRectFilled(ImVec2(0, 0), screen, top);
        return;   // done - skip the rest of the function
    }

    // gradient (also the base layer for pattern)
    draw->AddRectFilledMultiColor(ImVec2(0, 0), screen, top, top, bottom, bottom);

    if (wallpaperMode_ == "pattern") {
        // A grid of faint dots on top of the gradient.
        // Outer loop goes down the rows, inner loop goes across each row.
        const float spacing = 32.0f;                     // distance between dots
        ImU32 dot = IM_COL32(255, 255, 255, 40);         // white, mostly see-through
        for (float y = spacing / 2; y < screen.y; y += spacing) {
            for (float x = spacing / 2; x < screen.x; x += spacing) {
                draw->AddCircleFilled(ImVec2(x, y), 2.0f, dot);
            }
        }
    }
}

void Desktop::DrawClock (ImDrawList* draw, ImVec2 screen) {
    // Get the current time
    std::time_t now = std::time(0);
    std::tm* local = std::localtime(&now);

    // Format the time as a string
    char buffer[64];
    // Pick the format: 24h uses %H (00-23); 12h uses %I (01-12) plus %p (AM/PM)
    const char* format = clock24h_ ? "%A, %B %d, %Y || %H:%M:%S"
                                   : "%A, %B %d, %Y || %I:%M:%S %p";
    std::strftime(buffer, sizeof(buffer), format, local);

    // Size of the whole pill = text + padding on both sides
    const float margin = 10.0f;    // gap between the pill and the screen edge
    const float padding = 10.0f;   // gap between the text and the pill's edge
    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    ImVec2 pillSize = ImVec2(textSize.x + padding * 2, textSize.y + padding * 2);

    // Which corner? Work out left/right and top/bottom separately.
    bool onLeft = (clockCorner_ == "top_left" || clockCorner_ == "bottom_left");
    bool onTop  = (clockCorner_ == "top_left" || clockCorner_ == "top_right");

    float x = onLeft ? margin : screen.x - pillSize.x - margin;
    float y = onTop  ? topInset_ + margin : screen.y - bottomInset_ - pillSize.y - margin;

    // Draw the pill at (x, y), then the text inside it
    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + pillSize.x, y + pillSize.y),
                        IM_COL32(0, 0, 0, 160), 6.0f); // Semi-transparent black with rounded corners
    draw->AddText(ImVec2(x + padding, y + padding), IM_COL32(255, 255, 255, 255), buffer); // White color for the clock text
}

void Desktop::DrawPowerButton () {
    const float margin = 10.0f;
    const ImVec2 btnSize(56.0f, 32.0f);
    ImVec2 screen = ImGui::GetIO().DisplaySize;

    // Window is bigger than the button so nothing gets clipped
    const ImVec2 winSize(btnSize.x + 16.0f, btnSize.y + 16.0f);
    ImGui::SetNextWindowPos(ImVec2(margin - 8.0f, screen.y - bottomInset_ - btnSize.y - margin - 8.0f));
    ImGui::SetNextWindowSize(winSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("##pwr", nullptr, flags);

    // The default 8 px window padding now lands inside the extra space,
    // so the button sits exactly at (margin, screen.y - btnSize.y - margin)
    bool clicked = ImGui::InvisibleButton("##pwrbtn", btnSize);
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImVec2 c  = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);

    ImU32 bg = hovered ? IM_COL32(217, 51, 51, 255) : IM_COL32(25, 25, 30, 190);
    dl->AddRectFilled(p0, p1, bg, 8.0f);
    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 70), 8.0f);

    const float r = 9.0f;
    const float PI = 3.14159265f;
    dl->PathArcTo(c, r, -PI * 0.5f + 0.6f, -PI * 0.5f + 2.0f * PI - 0.6f);
    dl->PathStroke(IM_COL32(255, 255, 255, 255), 0, 2.0f);
    dl->AddLine(ImVec2(c.x, c.y - r - 2.0f), ImVec2(c.x, c.y - 1.0f),
                IM_COL32(255, 255, 255, 255), 2.0f);

    if (clicked && onShutdown_) onShutdown_();

    ImGui::End();
}