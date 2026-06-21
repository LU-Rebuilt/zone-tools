#include "client_context.h"
#include "path_resolve.h"

#include "netdevil/database/cdclient/cdclient.h"

#include <pugixml.hpp>
#include <sqlite3.h>

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

    res_dir_ = client_dir / "res";
    if (!fs::exists(res_dir_)) {
        res_dir_ = client_dir;
    }

    // Load locale
    auto locale_path = client_dir / "locale" / "locale.xml";
    if (fs::exists(locale_path)) {
        load_locale(locale_path);
    }

    // Use lu-assets CdClient to handle FDB→SQLite conversion (supports cdclient.fdb and ivantest.fdb)
    lu::assets::CdClient cdclient;
    if (!cdclient.open_from_client(client_dir)) {
        // No CDClient database found — fall back to scanning for .luz files
        scan_for_luz_files();
        return !zones_.empty();
    }

    // Find the sqlite file that CdClient opened/converted
    fs::path db_path;
    for (auto& candidate : {
        res_dir_ / "CDServer.sqlite",
        res_dir_ / "cdclient.sqlite",
        client_dir / "CDServer.sqlite",
        client_dir / "cdclient.sqlite",
        res_dir_ / "cdclient.converted.sqlite",
        client_dir / "cdclient.converted.sqlite",
        res_dir_ / "ivantest.converted.sqlite",
        client_dir / "ivantest.converted.sqlite",
    }) {
        if (fs::exists(candidate)) { db_path = candidate; break; }
    }
    if (db_path.empty()) {
        scan_for_luz_files();
        return !zones_.empty();
    }

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        scan_for_luz_files();
        return !zones_.empty();
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT zoneID, zoneName, DisplayDescription FROM ZoneTable ORDER BY zoneID";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ZoneInfo info;
        info.zone_id = sqlite3_column_int(stmt, 0);

        const char* zone_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.luz_path = zone_name ? zone_name : "";

        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string desc_str = desc ? desc : "";

        // Skip entries where zoneName is a locale key, not a file path
        if (info.luz_path.find("__removed") != std::string::npos ||
            (info.luz_path.find('.') == std::string::npos && !info.luz_path.empty())) {
            info.luz_path.clear();
        }

        // Display name: use DisplayDescription (direct or locale lookup)
        if (!desc_str.empty()) {
            auto it = locale_strings_.find(desc_str);
            if (it != locale_strings_.end()) {
                info.display_name = it->second;
            } else if (desc_str.find("ZoneTable_") == std::string::npos) {
                info.display_name = desc_str;
            }
        }

        // Try locale lookup with standard key pattern
        if (info.display_name.empty()) {
            std::string key = "ZoneTable_" + std::to_string(info.zone_id) + "_DisplayDescription";
            auto it = locale_strings_.find(key);
            if (it != locale_strings_.end()) {
                info.display_name = it->second;
            }
        }

        // Fallback: derive from luz path stem
        if (info.display_name.empty() && !info.luz_path.empty()) {
            fs::path p(info.luz_path);
            info.display_name = p.stem().string();
        }

        if (info.display_name.empty()) {
            info.display_name = "Zone " + std::to_string(info.zone_id);
        }

        // Only include zones whose LUZ file actually exists on disk
        if (!info.luz_path.empty()) {
            fs::path maps_dir = res_dir_ / "maps";
            fs::path resolved = resolve_case_insensitive(maps_dir, info.luz_path);
            if (fs::exists(resolved)) {
                zones_.push_back(std::move(info));
            }
        }
    }

    sqlite3_finalize(stmt);

    // Build zone display name map
    for (auto& z : zones_) {
        zone_display_names_[z.zone_id] = z.display_name;
    }

    load_object_names(db);

    sqlite3_close(db);
    return true;
}

void ClientContext::load_object_names(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, displayName FROM Objects";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        uint32_t id = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* display = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        std::string display_str = (display && display[0]) ? display : "";
        std::string name_str = (name && name[0]) ? name : "";

        if (!display_str.empty()) {
            // Resolve locale key
            auto it = locale_strings_.find(display_str);
            lot_names_[id] = (it != locale_strings_.end()) ? it->second : display_str;
        } else if (!name_str.empty()) {
            lot_names_[id] = name_str;
        }
    }
    sqlite3_finalize(stmt);
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
