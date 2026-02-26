#pragma once

#include <string>

namespace ydisquette {

inline std::string normalizeCloudPath(std::string p) {
    while (!p.empty() && (p.back() == ' ' || p.back() == '\t')) p.pop_back();
    while (!p.empty() && (p.front() == ' ' || p.front() == '\t')) p.erase(0, 1);
    if (p.size() >= 5 && p.compare(0, 5, "disk:") == 0)
        p = p.substr(5);
    while (p.size() >= 2 && p[0] == '/' && p[1] == '/')
        p = p.substr(1);
    if (!p.empty() && p[0] != '/')
        p = "/" + p;
    return p;
}

inline bool isValidCloudPath(const std::string& path) {
    if (path.empty()) return false;
    std::string n = normalizeCloudPath(path);
    if (n.empty()) return false;
    if (n != "/" && n.back() == '/') return false;
    for (char c : n) {
        if (c == '\0' || c == '\n' || c == '\r' || c == '\t') return false;
    }
    return true;
}

}  // namespace ydisquette
