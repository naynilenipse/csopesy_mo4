#pragma once
// ---------------------------------------------------------------------------
// MockScreenApp: a dummy app with its own icon that opens a separate mock
// screen (a fake window with a sidebar, some cards and a few controls).
// Nothing in it does real work - it's here to show what an extra app looks like.
// ---------------------------------------------------------------------------
#include "apps/App.h"

class MockScreenApp : public App {
public:
    MockScreenApp();

    // Custom taskbar icon: a small monitor instead of the default letter tile.
    void DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max) const override;

protected:
    void RenderContents() override;

private:
    int   selectedPage_ = 0;
    bool  toggleA_      = true;
    bool  toggleB_      = false;
    float slider_       = 0.5f;
    char  input_[64]    = "";
};