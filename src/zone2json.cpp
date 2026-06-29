#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/luz/luz_reader.h"
#include "netdevil/zone/lvl/lvl_json.h"
#include "netdevil/zone/lvl/lvl_reader.h"
#include "path_resolve.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

static void print_help() {
    std::fprintf(stderr,
        "zone2json — Convert a LEGO Universe zone to JSON\n"
        "\n"
        "Usage: zone2json <input.luz> [output.json]\n"
        "\n"
        "Reads a LUZ file and all referenced LVL scene files, producing a single\n"
        "JSON file containing the full zone (LUZ + scenes). Scene LVL files are\n"
        "resolved relative to the LUZ directory with case-insensitive matching.\n"
        "\n"
        "If no output path is given, writes to <input>.json.\n"
        "\n"
        "Options:\n"
        "  -h, --help    Show this help message\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "-h" || arg1 == "--help") {
        print_help();
        return 0;
    }

    fs::path input(argv[1]);
    fs::path output = (argc >= 3) ? fs::path(argv[2]) : fs::path(input).replace_extension(".json");

    auto ext = input.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".luz") {
        std::fprintf(stderr, "Error: expected .luz file (got '%s')\n", ext.c_str());
        return 1;
    }

    nlohmann::json j;
    try {
        auto luz_data = read_file(input);
        auto luz = lu::assets::luz_parse(luz_data);

        fs::path luz_dir = input.parent_path();

        nlohmann::json scenes_json = nlohmann::json::object();
        for (uint32_t i = 0; i < static_cast<uint32_t>(luz.scenes.size()); ++i) {
            fs::path scene_path = zone_tools::resolve_case_insensitive(
                luz_dir, luz.scenes[i].filename);

            if (!fs::exists(scene_path)) {
                std::fprintf(stderr, "  Scene %u: %s (not found)\n",
                    i, luz.scenes[i].filename.c_str());
                continue;
            }

            auto lvl_data = read_file(scene_path);
            auto lvl = lu::assets::lvl_parse(lvl_data);
            scenes_json[std::to_string(i)] = lvl;

            std::fprintf(stderr, "  Scene %u: %s (%zu objects, %zu particles)\n",
                i, luz.scenes[i].filename.c_str(),
                lvl.objects.size(), lvl.particles.size());
        }

        j["luz"] = luz;
        j["scenes"] = scenes_json;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    std::ofstream out(output);
    if (!out) {
        std::fprintf(stderr, "Error: cannot write %s\n", output.string().c_str());
        return 1;
    }
    out << j.dump(2) << '\n';

    std::fprintf(stderr, "Wrote %s\n", output.string().c_str());
    return 0;
}
