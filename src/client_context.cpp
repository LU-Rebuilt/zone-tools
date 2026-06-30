#include "client_context.h"
#include "path_resolve.h"

#include <pugixml.hpp>

#include <algorithm>

namespace fs = std::filesystem;

namespace zone_editor {

void ClientContext::load_locale(const fs::path& locale_path) {
    locale_strings_.clear();
    pugi::xml_document doc;
    if (!doc.load_file(locale_path.c_str())) return;

    for (auto phrase : doc.select_nodes("//phrase")) {
        auto node = phrase.node();
        std::string id = node.attribute("id").as_string();
        if (id.empty()) continue;

        for (auto tr : node.children("translation")) {
            std::string locale = tr.attribute("locale").as_string();
            if (locale == "en_US") {
                locale_strings_[id] = tr.text().as_string();
                break;
            }
        }
    }
}

bool ClientContext::open(const fs::path& client_dir) {
    client_dir_ = client_dir;
    zones_.clear();

    // Some early clients nest everything under a "client/" subdirectory
    fs::path effective_root = client_dir;
    if (!fs::exists(client_dir / "res") &&
        !fs::exists(client_dir / "maps") &&
        fs::exists(client_dir / "client" / "maps")) {
        effective_root = client_dir / "client";
    }

    res_dir_ = effective_root / "res";
    if (!fs::exists(res_dir_)) {
        res_dir_ = effective_root; // clients that put maps/fdb directly at root
    }

    for (auto& candidate : {
        effective_root / "locale" / "locale.xml",
        res_dir_       / "locale" / "locale.xml",
        client_dir     / "locale" / "locale.xml",
    }) {
        if (fs::exists(candidate)) {
            load_locale(candidate);
            break;
        }
    }

    lu::assets::CdClient cdclient;
    if (!cdclient.open_from_client(client_dir)) {
        scan_for_luz_files();
        return !zones_.empty();
    }

    // ZoneTable → zone list (resolve column indices by name for schema robustness)
    auto zt_cols = cdclient.table_columns("ZoneTable");
    auto zt_col = [&](const char* name) -> int {
        for (int i = 0; i < (int)zt_cols.size(); ++i)
            if (zt_cols[i] == name) return i;
        return -1;
    };
    int zt_id   = zt_col("zoneID");
    int zt_name = zt_col("zoneName");
    int zt_desc = zt_col("DisplayDescription");

    cdclient.query_each("ZoneTable", "", {}, [&](const lu::assets::CdRow& row) {
        if (zt_id < 0 || zt_name < 0) return;

        ZoneInfo info;
        if (auto* v = std::get_if<int32_t>(&row[zt_id])) info.zone_id = *v;
        else return;

        std::string zone_name_col;
        if (auto* v = std::get_if<std::string>(&row[zt_name])) zone_name_col = *v;

        std::string desc_str;
        if (zt_desc >= 0 && zt_desc < (int)row.size()) {
            if (auto* v = std::get_if<std::string>(&row[zt_desc])) desc_str = *v;
        }

        // zoneName column is a file path; skip locale keys and entries without extensions
        if (zone_name_col.find("__removed") != std::string::npos ||
            (zone_name_col.find('.') == std::string::npos && !zone_name_col.empty())) {
            zone_name_col.clear();
        }
        info.luz_path = zone_name_col;

        // Display name: locale lookup on desc, or direct string
        if (!desc_str.empty()) {
            auto it = locale_strings_.find(desc_str);
            if (it != locale_strings_.end()) {
                info.display_name = it->second;
            } else if (desc_str.find("ZoneTable_") == std::string::npos) {
                info.display_name = desc_str;
            }
        }
        if (info.display_name.empty()) {
            std::string key = "ZoneTable_" + std::to_string(info.zone_id) + "_DisplayDescription";
            auto it = locale_strings_.find(key);
            if (it != locale_strings_.end()) info.display_name = it->second;
        }
        if (info.display_name.empty() && !info.luz_path.empty()) {
            info.display_name = fs::path(info.luz_path).stem().string();
        }
        if (info.display_name.empty()) {
            info.display_name = "Zone " + std::to_string(info.zone_id);
        }

        if (!info.luz_path.empty()) {
            fs::path maps_dir = res_dir_ / "maps";
            fs::path resolved = zone_tools::resolve_case_insensitive(maps_dir, info.luz_path);
            if (fs::exists(resolved)) {
                zones_.push_back(std::move(info));
            }
        }
    });

    for (auto& z : zones_) {
        zone_display_names_[z.zone_id] = z.display_name;
    }

    load_object_names(cdclient);

    return true;
}

void ClientContext::load_object_names(lu::assets::CdClient& db) {
    auto cols = db.table_columns("Objects");
    auto col = [&](const char* name) -> int {
        for (int i = 0; i < (int)cols.size(); ++i)
            if (cols[i] == name) return i;
        return -1;
    };
    int c_id      = col("id");
    int c_name    = col("name");
    int c_display = col("displayName");

    db.query_each("Objects", "", {}, [&](const lu::assets::CdRow& row) {
        if (c_id < 0) return;

        uint32_t id = 0;
        if (auto* v = std::get_if<int32_t>(&row[c_id])) id = static_cast<uint32_t>(*v);
        else return;

        std::string name_str;
        if (c_name >= 0 && c_name < (int)row.size()) {
            if (auto* v = std::get_if<std::string>(&row[c_name])) name_str = *v;
        }

        std::string display_str;
        if (c_display >= 0 && c_display < (int)row.size()) {
            if (auto* v = std::get_if<std::string>(&row[c_display])) display_str = *v;
        }

        // Primary: locale key Objects_{id}_name
        std::string locale_key = "Objects_" + std::to_string(id) + "_name";
        auto it = locale_strings_.find(locale_key);
        if (it != locale_strings_.end() && !it->second.empty()) {
            lot_names_[id] = it->second;
        } else if (!name_str.empty()) {
            lot_names_[id] = name_str;
        }
    });
}

std::string ClientContext::lot_name(uint32_t lot) const {
    auto it = lot_names_.find(lot);
    if (it != lot_names_.end()) return it->second;
    return {};
}

std::string ClientContext::zone_name(int32_t zone_id) const {
    auto it = zone_display_names_.find(zone_id);
    if (it != zone_display_names_.end()) return it->second;
    return {};
}

const char* ClientContext::path_type_name(uint32_t type) {
    switch (type) {
    case 0: return "NPC";
    case 1: return "Moving Platform";
    case 2: return "Property";
    case 3: return "Camera";
    case 4: return "Spawner";
    case 5: return "Showcase";
    case 6: return "Racing";
    case 7: return "Rail";
    default: return "Unknown";
    }
}

const char* ClientContext::path_behavior_name(uint32_t behavior) {
    switch (behavior) {
    case 0: return "Loop";
    case 1: return "Bounce";
    case 2: return "Once";
    default: return "Unknown";
    }
}

const char* ClientContext::node_type_name(uint32_t type) {
    switch (type) {
    case 0:  return "Environment";
    case 1:  return "Building";
    case 2:  return "Enemy";
    case 3:  return "NPC";
    case 4:  return "Rebuilder";
    case 5:  return "Spawned";
    case 6:  return "Cannon";
    case 7:  return "Bouncer";
    case 8:  return "Exhibit";
    case 9:  return "Moving Platform";
    case 10: return "Springpad";
    case 11: return "Sound";
    case 12: return "Particle";
    case 13: return "Placeholder";
    case 14: return "Error Marker";
    case 15: return "Player Start";
    default: return "Unknown";
    }
}

void ClientContext::scan_for_luz_files() {
    fs::path maps_dir = res_dir_ / "maps";
    if (!fs::exists(maps_dir)) return;

    for (auto& entry : fs::recursive_directory_iterator(maps_dir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".luz") continue;

        ZoneInfo info;
        info.zone_id = static_cast<int32_t>(zones_.size() + 1);
        info.display_name = entry.path().stem().string();
        info.luz_path = fs::relative(entry.path(), maps_dir).string();
        std::replace(info.luz_path.begin(), info.luz_path.end(), '\\', '/');
        zones_.push_back(std::move(info));
    }

    std::sort(zones_.begin(), zones_.end(),
        [](auto& a, auto& b) { return a.display_name < b.display_name; });
}

} // namespace zone_editor
