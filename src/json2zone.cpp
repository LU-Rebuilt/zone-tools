#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/luz/luz_writer.h"
#include "netdevil/zone/lvl/lvl_json.h"
#include "netdevil/zone/lvl/lvl_writer.h"
#include "path_resolve.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static bool write_file(const fs::path& path, const std::vector<uint8_t>& data) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

static void print_help() {
    std::fprintf(stderr,
        "json2zone — Convert JSON back to LEGO Universe zone files\n"
        "\n"
        "Usage: json2zone <input.json> <output_dir>\n"
        "\n"
        "Reads a zone JSON file (produced by zone2json) and writes the LUZ file\n"
        "and all LVL scene files into <output_dir>, preserving the original\n"
        "directory structure relative to the LUZ.\n"
        "\n"
        "Options:\n"
        "  -h, --help    Show this help message\n");
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help") {
            print_help();
            return 0;
        }
    }

    if (argc < 3) {
        print_help();
        return 1;
    }

    fs::path input(argv[1]);
    fs::path output_dir(argv[2]);

    std::ifstream in(input);
    if (!in) {
        std::fprintf(stderr, "Error: cannot open %s\n", input.string().c_str());
        return 1;
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error parsing JSON: %s\n", e.what());
        return 1;
    }
    in.close();

    try {
        if (!j.contains("luz")) {
            std::fprintf(stderr, "Error: JSON missing \"luz\" key\n");
            return 1;
        }

        auto luz = j.at("luz").get<lu::assets::LuzFile>();

        // Derive LUZ filename from raw_path or zone_name
        std::string luz_filename;
        if (!luz.raw_path.empty()) {
            fs::path p(luz.raw_path);
            luz_filename = p.stem().string() + ".luz";
        } else if (!luz.zone_name.empty()) {
            luz_filename = luz.zone_name + ".luz";
        } else {
            luz_filename = "zone.luz";
        }

        fs::path luz_path = output_dir / luz_filename;
        auto luz_data = lu::assets::luz_write(luz);
        if (!write_file(luz_path, luz_data)) {
            std::fprintf(stderr, "Error: cannot write %s\n", luz_path.string().c_str());
            return 1;
        }
        std::fprintf(stderr, "Wrote %s (%zu bytes)\n", luz_path.string().c_str(), luz_data.size());

        if (j.contains("scenes")) {
            for (auto& [key, val] : j.at("scenes").items()) {
                uint32_t idx = std::stoul(key);
                if (idx >= luz.scenes.size()) {
                    std::fprintf(stderr, "  Warning: scene index %u out of range\n", idx);
                    continue;
                }

                auto lvl = val.get<lu::assets::LvlFile>();
                auto lvl_data = lu::assets::lvl_write(lvl);

                std::string scene_filename = luz.scenes[idx].filename;
                std::replace(scene_filename.begin(), scene_filename.end(), '\\', '/');

                fs::path scene_path = output_dir / scene_filename;
                if (!write_file(scene_path, lvl_data)) {
                    std::fprintf(stderr, "Error: cannot write %s\n", scene_path.string().c_str());
                    return 1;
                }
                std::fprintf(stderr, "  Scene %u: %s (%zu bytes)\n",
                    idx, scene_filename.c_str(), lvl_data.size());
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
