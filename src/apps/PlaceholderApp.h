#pragma once
// ---------------------------------------------------------------------------
// PlaceholderApp: a simple window that just shows a few lines of text.
//
// Used for taskbar buttons 1 and 2 until their real screens are built.
// To swap one out: make a new class that inherits from App (see apps/App.h)
// and put it in Application's apps_ list instead of the placeholder.
// ---------------------------------------------------------------------------
#include <string>
#include <vector>
#include "apps/App.h"

class PlaceholderApp : public App {
public:
    PlaceholderApp(const char* name, ImU32 accentColor, float baseCpu, float baseMemoryMB,
                   std::vector<std::string> lines);

protected:
    void RenderContents() override;

private:
    std::vector<std::string> lines_;
};
