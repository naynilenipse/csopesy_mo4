#include "core/Config.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>

// Removes spaces/tabs from both ends of a string: "  abc  " -> "abc"
static std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

bool Config::Load(const std::string& path) {
    path_ = path;
    values_.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        std::printf("[Config] Could not open '%s' - using defaults.\n", path.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;   // skip blanks and comments

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;           // not a key=value line

        std::string key   = ToLower(Trim(line.substr(0, eq)));
        std::string value = Trim(line.substr(eq + 1));
        values_[key] = value;
    }

    std::printf("[Config] Loaded %zu settings from '%s'.\n", values_.size(), path.c_str());
    return true;
}

std::string Config::GetString(const std::string& key, const std::string& def) const {
    auto it = values_.find(key);
    return it != values_.end() ? it->second : def;
}

int Config::GetInt(const std::string& key, int def) const {
    auto it = values_.find(key);
    if (it == values_.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

float Config::GetFloat(const std::string& key, float def) const {
    auto it = values_.find(key);
    if (it == values_.end()) return def;
    try { return std::stof(it->second); } catch (...) { return def; }
}

bool Config::GetBool(const std::string& key, bool def) const {
    auto it = values_.find(key);
    if (it == values_.end()) return def;
    std::string v = ToLower(it->second);
    if (v == "true" || v == "yes" || v == "1" || v == "on")  return true;
    if (v == "false" || v == "no" || v == "0" || v == "off") return false;
    return def;
}

ImVec4 Config::GetColor(const std::string& key, ImVec4 def) const {
    auto it = values_.find(key);
    if (it == values_.end()) return def;

    std::string hex = it->second;
    if (!hex.empty() && hex[0] == '#') hex = hex.substr(1);
    if (hex.size() != 6 && hex.size() != 8) return def;

    try {
        unsigned long v = std::stoul(hex, nullptr, 16);
        if (hex.size() == 6) v = (v << 8) | 0xFF;   // no alpha given -> fully opaque
        return ImVec4(((v >> 24) & 0xFF) / 255.0f,
                      ((v >> 16) & 0xFF) / 255.0f,
                      ((v >> 8)  & 0xFF) / 255.0f,
                      ( v        & 0xFF) / 255.0f);
    } catch (...) {
        return def;
    }
}
