#include "apps/Placeholder2.h"
#include <cstdio>

Placeholder2::Placeholder2()
    : App("Video player", ImVec2(1100, 720), IM_COL32(60, 110, 200, 255)) {
    baseCpu_      = 1.0f;
    baseMemoryMB_ = 30.0f;
}

void Placeholder2::EnsureTextures() const {
    if (texturesTried_) return;
    texturesTried_ = true;
    icon_.Load("assets/smiley.png");         // taskbar icon
    frame_.Load("assets/video_frame.png");   // the "video" / UI image
}

// ===========================================================================
//  Taskbar icon
// ===========================================================================
void Placeholder2::DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const {
    EnsureTextures();
    draw->AddRectFilled(min, max, accent_, 6.0f);

    float pad = (max.x - min.x) * 0.12f;
    ImVec2 a(min.x + pad, min.y + pad);
    ImVec2 b(max.x - pad, max.y - pad);

    if (icon_.IsLoaded()) {
        draw->AddImage(icon_.ImId(), a, b);
        return;
    }

    // Fallback: draw a smiley with shapes if the PNG could not be loaded.
    ImVec2 c((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    float r = (b.x - a.x) * 0.5f;
    const ImU32 dark = IM_COL32(60, 45, 10, 255);
    draw->AddCircleFilled(c, r, IM_COL32(255, 214, 51, 255), 32);
    draw->AddCircle(c, r, dark, 32, 2.0f);
    draw->AddCircleFilled(ImVec2(c.x - r * 0.35f, c.y - r * 0.2f), r * 0.1f, dark);
    draw->AddCircleFilled(ImVec2(c.x + r * 0.35f, c.y - r * 0.2f), r * 0.1f, dark);
    draw->PathArcTo(ImVec2(c.x, c.y + r * 0.05f), r * 0.5f, 0.35f, 2.79f, 16);   // lower half = smile
    draw->PathStroke(dark, ImDrawFlags_None, 2.0f);
}

// ===========================================================================
//  Window contents
// ===========================================================================
void Placeholder2::RenderContents() {
    EnsureTextures();

    // ---- YouTube-ish dark theme, scoped to this window ----
    ImGui::PushStyleColor(ImGuiCol_WindowBg,      ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,       ImVec4(0.13f, 0.13f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.26f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f, 0.32f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.95f, 0.95f, 0.95f, 1.0f));

    // ---- Header: small icon + title ----
    if (icon_.IsLoaded()) {
        ImGui::Image(icon_.ImId(), ImVec2(28, 28));
        ImGui::SameLine();
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
    ImGui::TextUnformatted("ominous_smiley.mp4");
    ImGui::SameLine();
    ImGui::TextDisabled("  (fake)");

    // Small gap before the video so the header doesn't touch it
    ImGui::Dummy(ImVec2(0, 8));

    // ------------------------------------------------------------------
    //  Auto-fit video: pick the largest 16:9 box that fits BOTH the
    //  available width AND the height left over after reserving room
    //  for the timeline + controls + status line below it.
    // ------------------------------------------------------------------
    const float reservedBelow = 140.0f;   // timeline + buttons + text + spacings
    const float spacing       = ImGui::GetStyle().ItemSpacing.y;

    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y - reservedBelow - spacing;
    if (availH < 120.0f) availH = 120.0f;          // never go tiny

    // 16:9 box that fits inside (availW, availH)
    float videoW = availW;
    float videoH = videoW * 9.0f / 16.0f;
    if (videoH > availH) {
        videoH = availH;
        videoW = videoH * 16.0f / 9.0f;
    }

    // Center the video horizontally inside the available width
    float xOffset = (availW - videoW) * 0.5f;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    p0.x += xOffset;
    ImVec2 p1(p0.x + videoW, p0.y + videoH);
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (frame_.IsLoaded()) {
        // Cover-fit: keep the image's aspect ratio, crop the overflow.
        float imageAspect = (float)frame_.width / (float)frame_.height;
        float areaAspect  = videoW / videoH;
        ImVec2 uv0(0, 0), uv1(1, 1);
        if (imageAspect > areaAspect) {
            float visible = areaAspect / imageAspect;
            uv0.x = (1.0f - visible) * 0.5f;
            uv1.x = uv0.x + visible;
        } else {
            float visible = imageAspect / areaAspect;
            uv0.y = (1.0f - visible) * 0.5f;
            uv1.y = uv0.y + visible;
        }
        draw->AddImageRounded(frame_.ImId(), p0, p1, uv0, uv1, IM_COL32_WHITE, 8.0f);
    } else {
        draw->AddRectFilled(p0, p1, IM_COL32(20, 20, 20, 255), 8.0f);
        const char* msg = "Put a PNG at assets/video_frame.png";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        draw->AddText(ImVec2(p0.x + (videoW - ts.x) * 0.5f, p0.y + (videoH - ts.y) * 0.5f),
                      IM_COL32(150, 155, 170, 255), msg);
    }

    // Reserve exactly the space we drew into, then advance past it
    ImGui::Dummy(ImVec2(availW, videoH));
    ImGui::Dummy(ImVec2(0, 6));

    // ---- Timeline: 0:44 [====----] 2:05 ---- (also centered)
    {
        const char* t0 = "0:44";
        const char* t1 = "2:05";
        float t0w = ImGui::CalcTextSize(t0).x;
        float t1w = ImGui::CalcTextSize(t1).x;
        float spacingX = ImGui::GetStyle().ItemSpacing.x;
        float barW = availW - t0w - t1w - spacingX * 4.0f;
        if (barW < 80.0f) barW = 80.0f;

        float rowW = t0w + spacingX + barW + spacingX + t1w;
        float xOff = (availW - rowW) * 0.5f;
        if (xOff > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOff);

        ImGui::TextDisabled("%s", t0);
        ImGui::SameLine();
        ImGui::ProgressBar(0.35f, ImVec2(barW, 6.0f), "");
        ImGui::SameLine();
        ImGui::TextDisabled("%s", t1);
    }
    ImGui::Dummy(ImVec2(0, 10));

    // ---- Playback controls (icon-shaped, centered) ----
    auto iconButton = [](const char* label, float w, float h) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        bool clicked = ImGui::Button(label, ImVec2(w, h));
        ImGui::PopStyleVar();
        return clicked;
    };

    // Button widths must match the ones used inside iconButton(...)
    const float wPrev = 48, hBtn = 40;
    const float wPlay = 64;
    const float wNext = 48;
    const float wMute = 48;
    float spacingX = ImGui::GetStyle().ItemSpacing.x;
    float rowWidth = wPrev + wPlay + wNext + wMute + spacingX * 3.0f;
    float xCentered = (availW - rowWidth) * 0.5f;
    if (xCentered > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xCentered);

    if (iconButton("|<", wPrev, hBtn)) lastAction_ = "Prev";
    ImGui::SameLine();
    if (iconButton(playing_ ? "||###play" : ">###play", wPlay, hBtn)) {
        playing_ = !playing_;
        lastAction_ = playing_ ? "Play" : "Pause";
    }
    ImGui::SameLine();
    if (iconButton(">|", wNext, hBtn)) lastAction_ = "Next";
    ImGui::SameLine();
    if (iconButton(muted_ ? "M###mute" : "S###mute", wMute, hBtn)) {
        muted_ = !muted_;
        lastAction_ = muted_ ? "Mute" : "Unmute";
    }

    ImGui::Dummy(ImVec2(0, 6));

    // ---- Status line (centered) ----
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                "Last action: %s", lastAction_);
    float tw = ImGui::CalcTextSize(buf).x;
    float xOff = (availW - tw) * 0.5f;
    if (xOff > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOff);
    ImGui::TextDisabled("%s", buf);

    ImGui::PopStyleColor(6);
}