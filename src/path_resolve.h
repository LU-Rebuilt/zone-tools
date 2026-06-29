#pragma once

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>

namespace zone_tools {

inline std::filesystem::path resolve_case_insensitive(
    const std::filesystem::path& base, const std::string& relative) {
    namespace fs = std::filesystem;

    std::string rel = relative;
    std::replace(rel.begin(), rel.end(), '\\', '/');

    fs::path current = base;
    std::istringstream stream(rel);
    std::string component;

    while (std::getline(stream, component, '/')) {
        if (component.empty()) continue;

        std::string lower_comp = component;
        std::transform(lower_comp.begin(), lower_comp.end(), lower_comp.begin(), ::tolower);

        bool found = false;
        if (fs::exists(current) && fs::is_directory(current)) {
            for (auto& entry : fs::directory_iterator(current)) {
                std::string entry_name = entry.path().filename().string();
                std::string lower_entry = entry_name;
                std::transform(lower_entry.begin(), lower_entry.end(), lower_entry.begin(), ::tolower);
                if (lower_entry == lower_comp) {
                    current = entry.path();
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            current /= component;
        }
    }
    return current;
}

} // namespace zone_tools
