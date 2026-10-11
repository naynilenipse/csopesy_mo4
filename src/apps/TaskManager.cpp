#include "apps/TaskManager.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>

// Dummy machine specs (only used for display)
static const float kTotalMemoryMB    = 16384.0f;   // 16 GB
static const float kSystemReservedMB = 4300.0f;    // "used by the OS" on top of the listed processes

// Column IDs, so the sort code knows which column was clicked
enum Column { ColName, ColStatus, ColCpu, ColMemory, ColDisk, ColNetwork, ColPid, ColUser };

// ===========================================================================
//  Small helpers
// ===========================================================================

// "abc" is found in "xxABCxx" (ignores upper/lower case)
static bool ContainsIgnoreCase(const std::string& text, const char* needle) {
    std::string a = text, b = needle;
    for (char& c : a) c = (char)std::tolower((unsigned char)c);
    for (char& c : b) c = (char)std::tolower((unsigned char)c);
    return a.find(b) != std::string::npos;
}

// Text pushed to the right edge of the current table cell (numbers read better that way)
static void RightAlignedText(const char* text) {
    float width = ImGui::CalcTextSize(text).x;
    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > width) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - width);
    ImGui::TextUnformatted(text);
}

// Windows shades busy cells darker yellow/orange. fraction: 0 = idle, 1 = very busy
static ImU32 HeatColor(float fraction) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    return IM_COL32(255, 170, 0, (int)(18 + 130 * fraction));
}

// One numeric cell: shaded background + right-aligned value
static void UsageCell(const char* text, float fraction, bool selected) {
    if (!selected) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, HeatColor(fraction));
    RightAlignedText(text);
}

// A filled line graph like the Performance tab on Windows.
// values is a ring buffer: the oldest value is at index 'oldest'.
static void DrawGraph(const float* values, int count, int oldest, ImU32 color, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImGui::Dummy(size);   // reserve the space so the next widget goes below the graph

    draw->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 90));
    for (int i = 1; i < 10; i++) {   // faint 10 x 10 grid
        float gx = p0.x + size.x * i / 10.0f;
        float gy = p0.y + size.y * i / 10.0f;
        draw->AddLine(ImVec2(gx, p0.y), ImVec2(gx, p1.y), IM_COL32(255, 255, 255, 20));
        draw->AddLine(ImVec2(p0.x, gy), ImVec2(p1.x, gy), IM_COL32(255, 255, 255, 20));
    }

    ImU32 fill = (color & 0x00FFFFFF) | (60u << 24);   // same color, mostly see-through
    for (int i = 0; i + 1 < count; i++) {
        float v0 = std::clamp(values[(oldest + i) % count], 0.0f, 100.0f);
        float v1 = std::clamp(values[(oldest + i + 1) % count], 0.0f, 100.0f);
        float x0 = p0.x + size.x * i / (count - 1);
        float x1 = p0.x + size.x * (i + 1) / (count - 1);
        float y0 = p1.y - size.y * v0 / 100.0f;
        float y1 = p1.y - size.y * v1 / 100.0f;
        // Each slice under the line is a 4-sided shape, filled down to the bottom
        draw->AddQuadFilled(ImVec2(x0, p1.y), ImVec2(x0, y0), ImVec2(x1, y1), ImVec2(x1, p1.y), fill);
        draw->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), color, 1.5f);
    }
    draw->AddRect(p0, p1, color);
}

// ===========================================================================
//  Setup
// ===========================================================================
TaskManager::TaskManager()
    : App("Task Manager", ImVec2(780, 500), IM_COL32(38, 132, 90, 255)) {
    baseCpu_      = 1.6f;
    baseMemoryMB_ = 30.7f;

    //             name                            user               critical   cpu%  mem MB  disk  net
    AddBackground("System",                        "SYSTEM",          true,  { 0.4f,   0.1f, 0.1f, 0.0f });
    AddBackground("Desktop Window Manager",        "DWM-1",           true,  { 1.8f,  62.5f, 0.0f, 0.0f });
    AddBackground("CSOPESY Shell Host",            "user",            true,  { 0.6f,  48.2f, 0.0f, 0.0f });
    AddBackground("Antimalware Service",           "SYSTEM",          false, { 2.4f, 145.3f, 0.4f, 0.0f });
    AddBackground("Search Indexer",                "SYSTEM",          false, { 0.9f,  35.7f, 0.6f, 0.0f });
    AddBackground("Service Host: Local System",    "SYSTEM",          false, { 0.4f,  18.3f, 0.0f, 0.0f });
    AddBackground("Service Host: Network Service", "NETWORK SERVICE", false, { 0.2f,   9.6f, 0.0f, 0.3f });
    AddBackground("Audio Service",                 "LOCAL SERVICE",   false, { 0.3f,  12.4f, 0.0f, 0.0f });
    AddBackground("Clock Service",                 "LOCAL SERVICE",   false, { 0.1f,   3.2f, 0.0f, 0.0f });
    AddBackground("Input Service",                 "SYSTEM",          false, { 0.2f,   6.8f, 0.0f, 0.0f });
    AddBackground("Update Service",                "SYSTEM",          false, { 0.0f,  21.9f, 0.1f, 0.2f });
    AddBackground("Print Spooler",                 "SYSTEM",          false, { 0.0f,   7.1f, 0.0f, 0.0f });
    AddBackground("Runtime Broker",                "user",            false, { 0.0f,   5.6f, 0.0f, 0.0f });
}

void TaskManager::Load(const Config& config) {
    updateInterval_ = config.GetFloat("taskmgr_update_sec", updateInterval_);
}

void TaskManager::AddBackground(const char* name, const char* user, bool critical, Usage base) {
    Process p;
    p.name     = name;
    // System is PID 4, like Windows; the rest get spread-out multiples of 4
    p.pid      = (processes_.empty()) ? 4 : (nextPid_ += 4 * (1 + (int)(rng_() % 60)));
    p.user     = user;
    p.critical = critical;
    p.base     = base;
    p.now      = base;
    processes_.push_back(p);
}

void TaskManager::SetApps(const std::vector<App*>& apps) {
    for (App* app : apps) {
        Process p;
        p.name     = app->GetName();
        p.pid      = (nextPid_ += 4 * (1 + (int)(rng_() % 60)));
        p.user     = "user";
        p.critical = false;
        p.app      = app;
        p.base     = { app->GetBaseCpu(), app->GetBaseMemoryMB(), 0.0f, 0.0f };
        p.now      = p.base;
        processes_.push_back(p);
    }
}

// ===========================================================================
//  Dummy numbers
// ===========================================================================
void TaskManager::Tick(float deltaSeconds) {
    if (!historyStarted_) {   // first frame: fill the graphs with the starting values
        Totals t = ComputeTotals();
        for (int m = 0; m < MetricCount; m++)
            for (int i = 0; i < HistorySize; i++) history_[m][i] = t.value[m];
        historyStarted_ = true;
    }

    if (updateInterval_ <= 0.0f) return;   // 0 in the config = frozen numbers
    updateTimer_ += deltaSeconds;
    if (updateTimer_ < updateInterval_) return;
    updateTimer_ = 0.0f;

    for (Process& p : processes_) Wobble(p);

    Totals t = ComputeTotals();
    for (int m = 0; m < MetricCount; m++) history_[m][historyPos_] = t.value[m];
    historyPos_ = (historyPos_ + 1) % HistorySize;
}

// New "current" numbers near the base numbers, so the table looks alive
void TaskManager::Wobble(Process& p) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    auto between = [&](float lo, float hi) { return lo + (hi - lo) * chance(rng_); };

    p.now.cpu = p.base.cpu * between(0.4f, 1.6f);
    if (chance(rng_) < 0.04f) p.now.cpu += between(2.0f, 8.0f);   // occasional spike

    p.now.memMB = p.base.memMB * between(0.98f, 1.02f);

    // Disk and network are idle most of the time, with the odd burst
    p.now.diskMB = p.base.diskMB > 0 ? p.base.diskMB * between(0.0f, 2.0f)
                                     : (chance(rng_) < 0.03f ? between(0.1f, 1.5f) : 0.0f);
    p.now.netMb  = p.base.netMb > 0  ? p.base.netMb * between(0.0f, 2.0f)
                                     : (chance(rng_) < 0.02f ? between(0.1f, 0.8f) : 0.0f);
}

TaskManager::Totals TaskManager::ComputeTotals() const {
    Usage sum;
    for (const Process& p : processes_) {
        if (p.app && !p.app->IsRunning()) continue;
        sum.cpu    += p.now.cpu;
        sum.memMB  += p.now.memMB;
        sum.diskMB += p.now.diskMB;
        sum.netMb  += p.now.netMb;
    }
    Totals t;
    t.value[Cpu]     = std::min(100.0f, sum.cpu);
    t.value[Memory]  = std::min(100.0f, (kSystemReservedMB + sum.memMB) / kTotalMemoryMB * 100.0f);
    t.value[Disk]    = std::min(100.0f, sum.diskMB * 5.0f);    // pretend 20 MB/s = 100% busy
    t.value[Network] = std::min(100.0f, sum.netMb / 10.0f);    // 1 Gbps link
    return t;
}

bool TaskManager::IsListed(const Process& p) const {
    if (p.app && !p.app->IsRunning()) return false;   // closed apps disappear from the list
    if (search_[0] == '\0') return true;
    return ContainsIgnoreCase(p.name, search_) || std::to_string(p.pid).find(search_) == 0;
}

int TaskManager::CountRunning() const {
    int count = 0;
    for (const Process& p : processes_)
        if (!p.app || p.app->IsRunning()) count++;
    return count;
}

const char* TaskManager::StatusOf(const Process& p) {
    return (p.app && p.app->IsMinimized()) ? "Minimized" : "Running";
}

// Sorts by whichever column header was clicked (no sorting until one is clicked)
void TaskManager::SortRows(std::vector<Process*>& rows, const ImGuiTableSortSpecs* sort) const {
    if (!sort || sort->SpecsCount == 0) return;
    const ImGuiTableColumnSortSpecs& spec = sort->Specs[0];
    bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;

    std::stable_sort(rows.begin(), rows.end(), [&](const Process* a, const Process* b) {
        int cmp = 0;   // <0: a first, >0: b first
        switch (spec.ColumnUserID) {
            case ColName:    cmp = a->name.compare(b->name); break;
            case ColStatus:  cmp = std::string(StatusOf(*a)).compare(StatusOf(*b)); break;
            case ColUser:    cmp = std::string(a->user).compare(b->user); break;
            case ColPid:     cmp = a->pid - b->pid; break;
            case ColCpu:     cmp = (a->now.cpu    > b->now.cpu)    - (a->now.cpu    < b->now.cpu);    break;
            case ColMemory:  cmp = (a->now.memMB  > b->now.memMB)  - (a->now.memMB  < b->now.memMB);  break;
            case ColDisk:    cmp = (a->now.diskMB > b->now.diskMB) - (a->now.diskMB < b->now.diskMB); break;
            case ColNetwork: cmp = (a->now.netMb  > b->now.netMb)  - (a->now.netMb  < b->now.netMb);  break;
        }
        return ascending ? cmp < 0 : cmp > 0;
    });
}

void TaskManager::EndTask(int pid) {
    for (size_t i = 0; i < processes_.size(); i++) {
        Process& p = processes_[i];
        if (p.pid != pid) continue;
        if (p.critical) return;

        if (p.app) p.app->Close();                      // closes the real window too
        else       processes_.erase(processes_.begin() + i);

        if (selectedPid_ == pid) selectedPid_ = -1;
        return;
    }
}

// ===========================================================================
//  The window
// ===========================================================================
void TaskManager::RenderContents() {
    Tick(ImGui::GetIO().DeltaTime);

    // Search box across the top, like Windows 11
    ImGui::SetNextItemWidth(std::min(320.0f, ImGui::GetContentRegionAvail().x));
    ImGui::InputTextWithHint("##search", "Type a name or PID to search", search_, sizeof(search_));

    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Processes"))    { RenderProcessesTab();                  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Performance"))  { RenderPerformanceTab();                ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("App history"))  { RenderPlaceholderTab("App history");   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Startup apps")) { RenderPlaceholderTab("Startup apps");  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Users"))        { RenderPlaceholderTab("Users");         ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Details"))      { RenderDetailsTab();                    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Services"))     { RenderPlaceholderTab("Services");      ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    // Ending a task removes rows, so it waits until the table is done drawing
    if (endTaskPid_ != -1) {
        EndTask(endTaskPid_);
        endTaskPid_ = -1;
    }
}

// ===========================================================================
//  Processes tab
// ===========================================================================
void TaskManager::RenderProcessesTab() {
    Totals t = ComputeTotals();

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                            ImGuiTableFlags_SortTristate | ImGuiTableFlags_BordersInnerV |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY;
    float footer = ImGui::GetFrameHeightWithSpacing();
    if (!ImGui::BeginTable("##processes", 6, flags, ImVec2(0, -footer))) return;

    float numWidth = ImGui::CalcTextSize("0000.0 Mbps").x;
    ImGui::TableSetupColumn("Name",    ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide, 0, ColName);
    ImGui::TableSetupColumn("Status",  ImGuiTableColumnFlags_WidthFixed, numWidth, ColStatus);
    ImGui::TableSetupColumn("CPU",     ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, numWidth, ColCpu);
    ImGui::TableSetupColumn("Memory",  ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, numWidth, ColMemory);
    ImGui::TableSetupColumn("Disk",    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, numWidth, ColDisk);
    ImGui::TableSetupColumn("Network", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, numWidth, ColNetwork);
    ImGui::TableSetupScrollFreeze(0, 2);   // the totals row + header row stay put when scrolling

    // Totals row above the column names, e.g. "23%" over "CPU"
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    for (int m = 0; m < MetricCount; m++) {
        ImGui::TableSetColumnIndex(2 + m);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f%%", t.value[m]);
        RightAlignedText(buf);
    }
    ImGui::TableHeadersRow();

    const ImGuiTableSortSpecs* sort = ImGui::TableGetSortSpecs();
    RenderProcessGroup("Apps", true, sort);
    RenderProcessGroup("Background processes", false, sort);

    ImGui::EndTable();
    RenderEndTaskButton();
}

// A collapsible group ("Apps (3)") with its rows underneath
void TaskManager::RenderProcessGroup(const char* label, bool apps, const ImGuiTableSortSpecs* sort) {
    std::vector<Process*> rows;
    for (Process& p : processes_)
        if ((p.app != nullptr) == apps && IsListed(p)) rows.push_back(&p);
    if (rows.empty()) return;
    SortRows(rows, sort);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    char text[64];
    std::snprintf(text, sizeof(text), "%s (%d)###%s", label, (int)rows.size(), label);   // ### keeps the ID fixed
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::TreeNodeEx(text, flags)) {
        for (Process* p : rows) RenderProcessRow(*p);
        ImGui::TreePop();
    }
}

void TaskManager::RenderProcessRow(Process& p) {
    bool selected = (p.pid == selectedPid_);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    // One invisible-ish selectable covering the whole row makes it clickable
    ImGui::PushID(p.pid);
    if (ImGui::Selectable("##row", selected,
                          ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
        selectedPid_ = p.pid;
    if (ImGui::BeginPopupContextItem("##menu")) {   // right-click menu
        selectedPid_ = p.pid;
        ImGui::BeginDisabled(p.critical);
        if (ImGui::MenuItem("End task")) endTaskPid_ = p.pid;
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    ImGui::PopID();

    // Icon + name, drawn on top of the selectable
    ImGui::SameLine();
    float icon = ImGui::GetTextLineHeight();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (p.app) {
        p.app->DrawIcon(draw, pos, ImVec2(pos.x + icon, pos.y + icon));
    } else {   // generic "program window" icon for background processes
        draw->AddRect(ImVec2(pos.x + 1, pos.y + 2), ImVec2(pos.x + icon - 1, pos.y + icon - 2),
                      IM_COL32(150, 160, 175, 255), 2.0f);
        draw->AddRectFilled(ImVec2(pos.x + 1, pos.y + 2), ImVec2(pos.x + icon - 1, pos.y + 5),
                            IM_COL32(150, 160, 175, 255), 2.0f);
    }
    ImGui::Dummy(ImVec2(icon, icon));
    ImGui::SameLine();
    ImGui::TextUnformatted(p.name.c_str());

    ImGui::TableSetColumnIndex(1);
    if (p.app && p.app->IsMinimized()) ImGui::TextDisabled("Minimized");

    char buf[32];
    ImGui::TableSetColumnIndex(2);
    std::snprintf(buf, sizeof(buf), "%.1f%%", p.now.cpu);
    UsageCell(buf, p.now.cpu / 20.0f, selected);

    ImGui::TableSetColumnIndex(3);
    std::snprintf(buf, sizeof(buf), "%.1f MB", p.now.memMB);
    UsageCell(buf, p.now.memMB / 400.0f, selected);

    ImGui::TableSetColumnIndex(4);
    std::snprintf(buf, sizeof(buf), "%.1f MB/s", p.now.diskMB);
    UsageCell(buf, p.now.diskMB / 2.0f, selected);

    ImGui::TableSetColumnIndex(5);
    std::snprintf(buf, sizeof(buf), "%.1f Mbps", p.now.netMb);
    UsageCell(buf, p.now.netMb / 2.0f, selected);
}

// Footer: process count on the left, "End task" on the right
void TaskManager::RenderEndTaskButton() {
    const Process* sel = nullptr;
    for (const Process& p : processes_)
        if (p.pid == selectedPid_ && IsListed(p)) sel = &p;

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Processes: %d", CountRunning());

    const char* label = "End task";
    float width = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - width));

    ImGui::BeginDisabled(!sel || sel->critical);
    if (ImGui::Button(label)) endTaskPid_ = selectedPid_;
    ImGui::EndDisabled();

    if (sel && sel->critical && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Access is denied. %s is a system process.", sel->name.c_str());
}

// ===========================================================================
//  Performance tab
// ===========================================================================
void TaskManager::RenderPerformanceTab() {
    Totals t = ComputeTotals();
    float usedGB = kTotalMemoryMB * t.value[Memory] / 100.0f / 1024.0f;
    float totalGB = kTotalMemoryMB / 1024.0f;

    static const char* titles[MetricCount] = { "CPU", "Memory", "Disk 0 (C:)", "Ethernet" };
    static const char* models[MetricCount] = { "CSOPESY Virtual CPU @ 3.20GHz", "16.0 GB DDR4",
                                               "Virtual SSD 256 GB", "Virtual Ethernet Adapter" };
    static const ImU32 colors[MetricCount] = { IM_COL32(17, 125, 187, 255), IM_COL32(139, 18, 174, 255),
                                               IM_COL32(77, 166, 12, 255),  IM_COL32(167, 79, 1, 255) };

    // Left: list of devices with their current value
    ImGui::BeginChild("##perfList", ImVec2(ImGui::CalcTextSize("Disk 0 (C:)  00.0/00.0 GB").x, 0),
                      ImGuiChildFlags_Borders);
    for (int m = 0; m < MetricCount; m++) {
        char label[96];
        if (m == Memory) std::snprintf(label, sizeof(label), "%s\n%.1f/%.1f GB (%.0f%%)", titles[m], usedGB, totalGB, t.value[m]);
        else             std::snprintf(label, sizeof(label), "%s\n%.0f%%", titles[m], t.value[m]);
        ImGui::PushID(m);
        if (ImGui::Selectable(label, perfPage_ == m, 0, ImVec2(0, ImGui::GetTextLineHeight() * 2.2f)))
            perfPage_ = m;
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Right: big graph + stats for the selected device
    ImGui::SameLine();
    ImGui::BeginChild("##perfGraph");
    ImGui::TextUnformatted(titles[perfPage_]);
    ImGui::SameLine();
    ImGui::TextDisabled("  %s", models[perfPage_]);
    if (updateInterval_ > 0.0f) ImGui::TextDisabled("%% Utilization over %.0f seconds", HistorySize * updateInterval_);
    else                        ImGui::TextDisabled("%% Utilization (updates turned off)");

    float statsHeight = ImGui::GetTextLineHeightWithSpacing() * 3 + ImGui::GetStyle().ItemSpacing.y * 2;
    ImVec2 graphSize(ImGui::GetContentRegionAvail().x, std::max(60.0f, ImGui::GetContentRegionAvail().y - statsHeight));
    DrawGraph(history_[perfPage_], HistorySize, historyPos_, colors[perfPage_], graphSize);

    // Stats under the graph, as label/value pairs
    int seconds = (int)ImGui::GetTime();
    char uptime[32];
    std::snprintf(uptime, sizeof(uptime), "%d:%02d:%02d", seconds / 3600, (seconds / 60) % 60, seconds % 60);
    char a[32], b[32], c[32];
    std::vector<std::pair<const char*, std::string>> stats;
    switch (perfPage_) {
        case Cpu:
            std::snprintf(a, sizeof(a), "%.0f%%", t.value[Cpu]);
            std::snprintf(b, sizeof(b), "%d", CountRunning());
            stats = { { "Utilization", a }, { "Speed", "3.20 GHz" }, { "Processes", b },
                      { "Up time", uptime }, { "Cores", "4" }, { "Logical processors", "8" } };
            break;
        case Memory:
            std::snprintf(a, sizeof(a), "%.1f GB", usedGB);
            std::snprintf(b, sizeof(b), "%.1f GB", totalGB - usedGB);
            std::snprintf(c, sizeof(c), "%.1f GB", totalGB);
            stats = { { "In use", a }, { "Available", b }, { "Total", c }, { "Speed", "3200 MT/s" } };
            break;
        case Disk:
            std::snprintf(a, sizeof(a), "%.0f%%", t.value[Disk]);
            std::snprintf(b, sizeof(b), "%.1f MB/s", t.value[Disk] / 5.0f * 0.7f);
            std::snprintf(c, sizeof(c), "%.1f MB/s", t.value[Disk] / 5.0f * 0.3f);
            stats = { { "Active time", a }, { "Read speed", b }, { "Write speed", c }, { "Capacity", "256 GB" } };
            break;
        case Network:
            std::snprintf(a, sizeof(a), "%.1f Mbps", t.value[Network] * 10.0f * 0.3f);
            std::snprintf(b, sizeof(b), "%.1f Mbps", t.value[Network] * 10.0f * 0.7f);
            stats = { { "Send", a }, { "Receive", b }, { "Link speed", "1.0 Gbps" } };
            break;
    }
    if (ImGui::BeginTable("##stats", 4, ImGuiTableFlags_SizingStretchSame)) {
        for (auto& s : stats) {
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s", s.first);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(s.second.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

// ===========================================================================
//  Details tab: every process in one flat, sortable list
// ===========================================================================
void TaskManager::RenderDetailsTab() {
    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY;
    float footer = ImGui::GetFrameHeightWithSpacing();
    if (!ImGui::BeginTable("##details", 6, flags, ImVec2(0, -footer))) return;

    ImGui::TableSetupColumn("Name",      ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0, ColName);
    ImGui::TableSetupColumn("PID",       ImGuiTableColumnFlags_WidthFixed, 0, ColPid);
    ImGui::TableSetupColumn("Status",    ImGuiTableColumnFlags_WidthFixed, 0, ColStatus);
    ImGui::TableSetupColumn("User name", ImGuiTableColumnFlags_WidthFixed, 0, ColUser);
    ImGui::TableSetupColumn("CPU",       ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 0, ColCpu);
    ImGui::TableSetupColumn("Memory",    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 0, ColMemory);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    std::vector<Process*> rows;
    for (Process& p : processes_)
        if (IsListed(p)) rows.push_back(&p);
    SortRows(rows, ImGui::TableGetSortSpecs());

    for (Process* p : rows) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(p->pid);
        if (ImGui::Selectable(p->name.c_str(), p->pid == selectedPid_, ImGuiSelectableFlags_SpanAllColumns))
            selectedPid_ = p->pid;
        ImGui::PopID();
        ImGui::TableNextColumn(); ImGui::Text("%d", p->pid);
        ImGui::TableNextColumn(); ImGui::TextUnformatted(StatusOf(*p));
        ImGui::TableNextColumn(); ImGui::TextUnformatted(p->user);
        ImGui::TableNextColumn(); ImGui::Text("%02.0f", p->now.cpu);   // Windows shows whole numbers here
        ImGui::TableNextColumn(); ImGui::Text("%.0f K", p->now.memMB * 1024.0f);
    }
    ImGui::EndTable();
    RenderEndTaskButton();
}

void TaskManager::RenderPlaceholderTab(const char* what) {
    ImGui::Spacing();
    ImGui::TextDisabled("%s: placeholder tab (not part of this mock-up).", what);
}

// ===========================================================================
//  Taskbar icon: green tile with a white "heartbeat" line
// ===========================================================================
void TaskManager::DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const {
    draw->AddRectFilled(min, max, accent_, 6.0f);

    float w = max.x - min.x, h = max.y - min.y;
    const float shape[][2] = { { 0.12f, 0.55f }, { 0.35f, 0.55f }, { 0.45f, 0.28f },
                               { 0.58f, 0.78f }, { 0.68f, 0.55f }, { 0.88f, 0.55f } };
    ImVec2 points[6];
    for (int i = 0; i < 6; i++)
        points[i] = ImVec2(min.x + w * shape[i][0], min.y + h * shape[i][1]);
    draw->AddPolyline(points, 6, IM_COL32(255, 255, 255, 255), 0, std::max(1.0f, w * 0.08f));
}
