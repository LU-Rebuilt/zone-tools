#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/luz/luz_writer.h"
#include "netdevil/zone/lvl/lvl_json.h"
#include "netdevil/zone/lvl/lvl_writer.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static void print_help() {
    std::fprintf(stderr,
        "json2zone — Convert JSON back to LEGO Universe zone files\n"
        "\n"
        "Usage: json2zone <input.json> <output.luz|output.lvl>\n"
        "\n"
        "Converts a JSON file back to a LUZ (zone) or LVL (scene/level) binary file.\n"
        "The output format is determined by the output file extension.\n"
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
    fs::path output(argv[2]);

    auto ext = output.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".luz" && ext != ".lvl") {
        std::fprintf(stderr, "Error: output extension must be .luz or .lvl (got '%s')\n", ext.c_str());
        return 1;
    }

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

    std::vector<uint8_t> data;
    try {
        if (ext == ".luz") {
            data = lu::assets::luz_write(j.get<lu::assets::LuzFile>());
        } else {
            data = lu::assets::lvl_write(j.get<lu::assets::LvlFile>());
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::fprintf(stderr, "Error: cannot write %s\n", output.string().c_str());
        return 1;
    }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());

    std::fprintf(stderr, "Wrote %s (%zu bytes)\n", output.string().c_str(), data.size());
    return 0;
}
