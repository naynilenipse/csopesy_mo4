#include "shell/Desktop.h"
#include <ctime>

void Desktop::Load (const Config& config) {
    // Load the desktop configuration from the provided config object, like the wallpaper path, gradient colors, etc.
    gradientTop_ = config.GetColor("wallpaper_top", gradientTop_);
    gradientBottom_ = config.GetColor("wallpaper_bottom", gradientBottom_);
}

void Desktop::Render () {
    ImDrawList* draw = ImGui::GetBackgroundDrawList();// Get the draw list for the background layer
    ImVec2 screen = ImGui::GetIO().DisplaySize; // Get the size of the display

    // Draw the wallpaper
    DrawWallpaper(draw, screen);
    DrawClock(draw, screen);
}

void Desktop::DrawWallpaper (ImDrawList* draw, ImVec2 screen) {
    //Drawing functiions want colors packed as ImU32, so we convert the gradient colors from ImVec4 to ImU32
    ImU32 top = ImGui::ColorConvertFloat4ToU32(gradientTop_);
    ImU32 bottom = ImGui::ColorConvertFloat4ToU32(gradientBottom_);


    draw->AddRectFilledMultiColor(ImVec2(0, 0), screen, top, top, bottom, bottom);
}

void Desktop::DrawClock (ImDrawList* draw, ImVec2 screen) {
    // Get the current time
    std::time_t now = std::time(0);
    std::tm* local = std::localtime(&now);

    // Format the time as a string
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%A, %B %d, %Y || %I:%M:%S %p", local);

    // Draw the clock in the top-right corner of the screen
    const float margin = 10.0f; // Padding from the edges
    const float padding = 10.0f; // Padding from the edges
    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    ImVec2 position = ImVec2(screen.x - textSize.x - margin, margin+padding); // margin pixels from the right and top edges

    // Draw the pill background for the clock
    draw->AddRectFilled(ImVec2(position.x - padding, position.y - padding),
                        ImVec2(position.x + textSize.x + padding, position.y + textSize.y + padding),
                        IM_COL32(0, 0, 0, 160), 6.0f); // Semi-transparent black with rounded corners
    draw->AddText(position, IM_COL32(255, 255, 255, 255), buffer); // White color for the clock text
}