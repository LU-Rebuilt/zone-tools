#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct sqlite3;

namespace zone_editor {

struct ZoneInfo {
    int32_t zone_id{};
    std::string display_name;
    std::string luz_path;
};

class ClientContext {
public:
    bool open(const std::filesystem::path& client_dir);

    const std::filesystem::path& res_dir() const { return res_dir_; }
    const std::vector<ZoneInfo>& zones() const { return zones_; }

    std::string lot_name(uint32_t lot) const;
    std::string zone_name(int32_t zone_id) const;

    static const char* path_type_name(uint32_t type);
    static const char* path_behavior_name(uint32_t behavior);
    static const char* node_type_name(uint32_t type);

private:
    void load_locale(const std::filesystem::path& locale_path);
    void load_object_names(sqlite3* db);
    void scan_for_luz_files();

    std::filesystem::path client_dir_;
    std::filesystem::path res_dir_;
    std::vector<ZoneInfo> zones_;
    std::unordered_map<std::string, std::string> locale_strings_;
    std::unordered_map<uint32_t, std::string> lot_names_;
    std::unordered_map<int32_t, std::string> zone_display_names_;
};

} // namespace zone_editor
