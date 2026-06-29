#include "zone_document.h"
#include "path_resolve.h"

#include "netdevil/zone/luz/luz_reader.h"
#include "netdevil/zone/luz/luz_writer.h"
#include "netdevil/zone/lvl/lvl_reader.h"
#include "netdevil/zone/lvl/lvl_writer.h"

#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

namespace zone_editor {

static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

static bool write_file(const fs::path& path, const std::vector<uint8_t>& data) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

bool ZoneDocument::load_from_cdclient(const std::string& cdclient_luz_path, const fs::path& res_dir) {
    fs::path maps_dir = res_dir / "maps";
    fs::path resolved = zone_tools::resolve_case_insensitive(maps_dir, cdclient_luz_path);
    return load(resolved, res_dir);
}

bool ZoneDocument::load(const fs::path& luz_path, const fs::path& res_dir) {
    loaded_ = false;
    scenes_.clear();
    scene_paths_.clear();
    luz_modified_ = false;
    modified_scenes_.clear();

    luz_path_ = luz_path;
    res_dir_ = res_dir;

    auto luz_data = read_file(luz_path);
    if (luz_data.empty()) return false;

    luz_ = lu::assets::luz_parse(luz_data);

    fs::path luz_dir = luz_path.parent_path();

    for (uint32_t i = 0; i < static_cast<uint32_t>(luz_.scenes.size()); ++i) {
        fs::path scene_path = zone_tools::resolve_case_insensitive(luz_dir, luz_.scenes[i].filename);
        scene_paths_[i] = scene_path;

        if (!fs::exists(scene_path)) continue;

        auto lvl_data = read_file(scene_path);
        if (lvl_data.empty()) continue;

        try {
            scenes_[i] = lu::assets::lvl_parse(lvl_data);
        } catch (...) {
            // Gracefully handle unparseable LVLs
        }
    }

    loaded_ = true;
    return true;
}

bool ZoneDocument::save() {
    if (!loaded_) return false;

    if (luz_modified_) {
        auto data = lu::assets::luz_write(luz_);
        if (!write_file(luz_path_, data)) return false;
        luz_modified_ = false;
    }

    for (uint32_t idx : modified_scenes_) {
        if (!scenes_.count(idx) || !scene_paths_.count(idx)) continue;
        auto data = lu::assets::lvl_write(scenes_.at(idx));
        if (!write_file(scene_paths_.at(idx), data)) return false;
    }
    modified_scenes_.clear();
    return true;
}

bool ZoneDocument::save_as(const fs::path& dir) {
    if (!loaded_) return false;

    fs::path new_luz_path = dir / luz_path_.filename();
    auto luz_data = lu::assets::luz_write(luz_);
    if (!write_file(new_luz_path, luz_data)) return false;

    for (auto& [idx, scene] : scenes_) {
        if (!scene_paths_.count(idx)) continue;
        fs::path rel = fs::relative(scene_paths_.at(idx), luz_path_.parent_path());
        fs::path new_path = dir / rel;
        auto data = lu::assets::lvl_write(scene);
        if (!write_file(new_path, data)) return false;
    }

    return true;
}

} // namespace zone_editor
