#include "apps/PlaceholderApp.h"

#include <utility>

PlaceholderApp::PlaceholderApp(const char* name, ImU32 accentColor, float baseCpu, float baseMemoryMB,
                               std::vector<std::string> lines)
    : App(name, ImVec2(420, 260), accentColor), lines_(std::move(lines)) {
    baseCpu_      = baseCpu;
    baseMemoryMB_ = baseMemoryMB;
}

void PlaceholderApp::RenderContents() {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Placeholder screen");
    ImGui::Separator();
    for (const std::string& line : lines_)
        ImGui::TextWrapped("%s", line.c_str());
}
