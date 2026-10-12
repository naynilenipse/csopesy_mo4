#include "apps/MockScreenApp.h"

MockScreenApp::MockScreenApp()
    : App("Mock Screen", ImVec2(560, 360), IM_COL32(140, 70, 200, 255)) {
    baseCpu_      = 0.8f;
    baseMemoryMB_ = 30.5f;
}

void MockScreenApp::DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const {
    draw->AddRectFilled(min, max, accent_, 6.0f);

    // A tiny monitor: screen, stand and base
    float w = max.x - min.x, h = max.y - min.y;
    ImVec2 sMin(min.x + w * 0.18f, min.y + h * 0.20f);
    ImVec2 sMax(max.x - w * 0.18f, min.y + h * 0.62f);
    draw->AddRectFilled(sMin, sMax, IM_COL32(230, 240, 255, 255), 2.0f);
    float cx = (min.x + max.x) * 0.5f;
    draw->AddRectFilled(ImVec2(cx - w * 0.05f, sMax.y), ImVec2(cx + w * 0.05f, min.y + h * 0.74f),
                        IM_COL32(230, 240, 255, 255));
    draw->AddRectFilled(ImVec2(cx - w * 0.20f, min.y + h * 0.74f), ImVec2(cx + w * 0.20f, min.y + h * 0.80f),
                        IM_COL32(230, 240, 255, 255), 1.0f);
}

void MockScreenApp::RenderContents() {
    static const char* pages[] = { "Home", "Files", "Settings" };

    // ---- Left sidebar ----
    ImGui::BeginChild("##sidebar", ImVec2(120, 0), true);
    for (int i = 0; i < IM_ARRAYSIZE(pages); ++i)
        if (ImGui::Selectable(pages[i], selectedPage_ == i))
            selectedPage_ = i;
    ImGui::EndChild();

    ImGui::SameLine();

    // ---- Main area ----
    ImGui::BeginChild("##main", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.75f, 0.55f, 1.0f, 1.0f), "%s", pages[selectedPage_]);
    ImGui::Separator();

    if (selectedPage_ == 0) {
        ImGui::TextWrapped("This is a mock screen. None of these controls do anything yet.");
        ImGui::Spacing();
        for (int i = 0; i < 3; ++i) {
            if (i > 0) ImGui::SameLine();
            ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)i), ImVec2(120, 70), true);
            ImGui::Text("Card %d", i + 1);
            ImGui::TextDisabled("Dummy data");
            ImGui::EndChild();
        }
        ImGui::Spacing();
        ImGui::ProgressBar(0.65f, ImVec2(-1, 0), "65% (fake)");
    } else if (selectedPage_ == 1) {
        static const char* files[] = { "readme.txt", "report.docx", "photo.png", "notes.md" };
        for (const char* f : files)
            ImGui::BulletText("%s", f);
    } else {
        ImGui::Checkbox("Dummy option A", &toggleA_);
        ImGui::Checkbox("Dummy option B", &toggleB_);
        ImGui::SliderFloat("Dummy level", &slider_, 0.0f, 1.0f);
        ImGui::InputText("Dummy name", input_, sizeof(input_));
        ImGui::Button("Apply (does nothing)");
    }
    ImGui::EndChild();
}  