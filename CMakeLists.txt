# ---------------------------------------------------------------------------
# CSOPESY Desktop OS Mock-up - build script
#
# You normally never run this by hand. In VS Code, press F5 (or use the
# CMake Tools "Build" button) and it does everything for you.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)
project(CSOPESYDesktop CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Writes build/compile_commands.json so VS Code autocomplete understands the code.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put the .exe in the build folder root so it is easy to find.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# --- GLFW (creates the OS window + handles keyboard/mouse) -----------------
# Turn off the parts of GLFW we don't need so the build is faster.
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# --- Dear ImGui (draws all our UI) -----------------------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# --- Our program -----------------------------------------------------------
# Every .cpp file under src/ is compiled automatically, so when you add a new
# file you don't have to edit this list. (Re-run CMake configure if a new file
# isn't picked up.)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(CSOPESYDesktop ${APP_SOURCES})

target_include_directories(CSOPESYDesktop PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/stb
)
target_link_libraries(CSOPESYDesktop PRIVATE imgui glfw opengl32)

# Bundle the MinGW runtime into the .exe so it runs on any Windows PC
# without needing extra DLL files next to it.
if(MINGW)
    target_link_options(CSOPESYDesktop PRIVATE -static)
endif()
