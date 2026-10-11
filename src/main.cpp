// ---------------------------------------------------------------------------
// CSOPESY Desktop OS Mock-up - ENTRY POINT (the main function lives here)
//
// main() is intentionally tiny: all the real work is in core/Application.
// ---------------------------------------------------------------------------
#include <cstdio>
#include "core/Application.h"

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);   // print log messages immediately

    Application app;
    return app.Run();
}
