#pragma once
// ---------------------------------------------------------------------------
// TaskManager: a window made to look like the Windows Task Manager.
//
//   Processes tab   - table of "Apps" (our real desktop apps that are running)
//                     and "Background processes" (made-up), each with dummy
//                     CPU / Memory / Disk / Network numbers. Cells are shaded
//                     darker the higher the usage, like on Windows.
//                     Click a header to sort, type in the box to search,
//                     select a row and press "End task" to end it.
//   Performance tab - live graphs of the dummy totals
//   Details tab     - flat list with PIDs and user names
//   Other tabs      - placeholders
//
// All numbers are DUMMY values: base numbers below, plus a little random
// wobble every taskmgr_update_sec seconds so it looks alive.
// ---------------------------------------------------------------------------
#include <random>
#include <string>
#include <vector>
#include "apps/App.h"
#include "core/Config.h"

class TaskManager : public App {
public:
    TaskManager();

    void Load(const Config& config);

    // The desktop's apps. Running ones are listed under "Apps" and can be ended.
    void SetApps(const std::vector<App*>& apps);

    void DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const override;

protected:
    void RenderContents() override;

private:
    struct Usage {
        float cpu    = 0.0f;   // percent
        float memMB  = 0.0f;   // megabytes
        float diskMB = 0.0f;   // megabytes per second
        float netMb  = 0.0f;   // megabits per second
    };

    struct Process {
        std::string name;
        int         pid;
        const char* user;
        bool        critical;          // can't be ended ("Access is denied")
        App*        app = nullptr;     // set if this row is one of our desktop apps
        Usage       base;              // the dummy value it wobbles around
        Usage       now;               // what the table shows right now
    };

    // Whole-system usage in percent, indexed by Metric (used by the headers and graphs)
    enum Metric { Cpu, Memory, Disk, Network, MetricCount };
    struct Totals { float value[MetricCount]; };

    // Each graph on the Performance tab keeps the last HistorySize totals
    static const int HistorySize = 60;

    void   AddBackground(const char* name, const char* user, bool critical, Usage base);
    void   Tick(float deltaSeconds);       // wobble the numbers every updateInterval_
    void   Wobble(Process& p);
    Totals ComputeTotals() const;
    bool   IsListed(const Process& p) const;   // running and matching the search box
    int    CountRunning() const;           // ignores the search box, like Windows
    void   SortRows(std::vector<Process*>& rows, const ImGuiTableSortSpecs* sort) const;
    static const char* StatusOf(const Process& p);

    void RenderProcessesTab();
    void RenderProcessGroup(const char* label, bool apps, const ImGuiTableSortSpecs* sort);
    void RenderProcessRow(Process& p);
    void RenderPerformanceTab();
    void RenderDetailsTab();
    void RenderPlaceholderTab(const char* what);
    void RenderEndTaskButton();
    void EndTask(int pid);

    std::vector<Process> processes_;
    int   nextPid_ = 1000;
    int   selectedPid_ = -1;
    int   endTaskPid_  = -1;           // ended after the table finishes drawing
    char  search_[64] = "";
    int   perfPage_ = 0;               // which graph is selected on the Performance tab

    float updateInterval_ = 1.0f;      // taskmgr_update_sec; 0 = numbers never change
    float updateTimer_    = 0.0f;
    float history_[4][HistorySize] = {};
    int   historyPos_ = 0;
    bool  historyStarted_ = false;

    std::mt19937 rng_{ 2026 };         // fixed seed: every run starts the same way
};
