#pragma once

#include "netdevil/zone/luz/luz_types.h"
#include "netdevil/zone/lvl/lvl_types.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace zone_editor {

class ZoneDocument {
public:
    bool load(const std::filesystem::path& luz_path, const std::filesystem::path& res_dir);
    bool load_from_cdclient(const std::string& cdclient_luz_path, const std::filesystem::path& res_dir);
    bool save();
    bool save_as(const std::filesystem::path& dir);

    bool is_loaded() const { return loaded_; }
    const std::filesystem::path& luz_path() const { return luz_path_; }

    lu::assets::LuzFile& luz() { return luz_; }
    const lu::assets::LuzFile& luz() const { return luz_; }

    bool has_scene(uint32_t idx) const { return scenes_.count(idx) > 0; }
    lu::assets::LvlFile& scene(uint32_t idx) { return scenes_.at(idx); }
    const lu::assets::LvlFile& scene(uint32_t idx) const { return scenes_.at(idx); }
    uint32_t scene_count() const { return static_cast<uint32_t>(luz_.scenes.size()); }

    void mark_luz_modified() { luz_modified_ = true; }
    void mark_scene_modified(uint32_t idx) { modified_scenes_.insert(idx); }
    bool is_modified() const { return luz_modified_ || !modified_scenes_.empty(); }

private:
    bool loaded_ = false;
    std::filesystem::path luz_path_;
    std::filesystem::path res_dir_;

    lu::assets::LuzFile luz_;
    std::map<uint32_t, lu::assets::LvlFile> scenes_;
    std::map<uint32_t, std::filesystem::path> scene_paths_;

    bool luz_modified_ = false;
    std::set<uint32_t> modified_scenes_;
};

} // namespace zone_editor
