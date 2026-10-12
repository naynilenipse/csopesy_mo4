CSOPESY MO4 - Desktop-Style OS Mock-up
=======================================

Group members:
  - Espineli, Nyan
  - Guarin, Raine
  - Montaño, Rovin
  - Tolentino, Winelle

Entry point (main function): src/main.cpp

Built with C++17, GLFW 3.4, Dear ImGui 1.91.5, OpenGL 3 and stb_image.
All libraries are included in the external/ folder - no downloads needed.


ONE-TIME SETUP (Windows)
------------------------
1. Install the compiler + build tools (g++, CMake, Ninja, gdb) in one step.
   Open PowerShell and run:

       winget install BrechtSanders.WinLibs.MCF.UCRT

   Then CLOSE and REOPEN VS Code (and any terminals) so it sees the new tools.
   Check it worked - each of these should print a version:

       g++ --version
       cmake --version
       gdb --version

2. In VS Code, install the extension "C/C++" (by Microsoft, ms-vscode.cpptools).

HOW TO RUN
----------
1. VS Code -> File -> Open Folder -> select THIS folder (csopesy-desktop),
   not its parent folder.
2. Press F5 (or Run -> Start Debugging).
   The first build takes about 30 seconds; later builds are faster.
3. Close the program with its PWR button. The window's X button is disabled
   on purpose (the spec requires shutting down through PWR).

Ctrl+Shift+B builds without running (useful to check for errors).


CHANGING SETTINGS (no recompiling)
----------------------------------
Edit config/config.txt, save, and press F5 again.


TROUBLESHOOTING
---------------
- "The preLaunchTask 'Build' terminated with exit code 1":
  a compile error. Click "Show Errors" (or open the Problems tab) and fix
  the FIRST error listed - later ones are often caused by it.
  Click "Abort", not "Debug Anyway" (that runs the old version).
- "cmake/g++ is not recognized": step 1 of setup didn't finish, or VS Code
  wasn't restarted after installing.
- Changes not showing up: save all files (File -> Auto Save helps), and make
  sure the previous run was closed with PWR before pressing F5 again.
