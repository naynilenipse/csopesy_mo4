#pragma once
// ---------------------------------------------------------------------------
// Config: reads settings from a text file like this:
//
//     # lines starting with # are comments
//     window_width = 1280
//     taskbar_position = bottom
//
// WHY: the quiz says we can't recompile, only change parameters. So anything
// the quiz might change must come from this file, not be typed into the code.
//
// HOW TO USE (from anywhere in the program):
//     int w = config.GetInt("window_width", 1280);   // 1280 = default value
//
// The default is used when the key is missing or typed wrong, so the program
// never crashes because of a bad config file.
// ---------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include "imgui.h"   // for ImVec4 (a color: red, green, blue, alpha from 0 to 1)

class Config {
public:
    // Reads the file. Returns false if the file couldn't be opened
    // (the program still runs, it just uses all the defaults).
    bool Load(const std::string& path);

    std::string GetString(const std::string& key, const std::string& def) const;
    int         GetInt   (const std::string& key, int def) const;
    float       GetFloat (const std::string& key, float def) const;
    bool        GetBool  (const std::string& key, bool def) const;     // true/false, yes/no, 1/0
    ImVec4      GetColor (const std::string& key, ImVec4 def) const;   // "#RRGGBB" or "#RRGGBBAA"

    const std::string& GetPath() const { return path_; }

private:
    std::string path_;
    std::unordered_map<std::string, std::string> values_;   // key -> raw text value
};
