#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/luz/luz_reader.h"
#include "netdevil/zone/lvl/lvl_json.h"
#include "netdevil/zone/lvl/lvl_reader.h"

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
        "zone2json — Convert LEGO Universe zone files to JSON\n"
        "\n"
        "Usage: zone2json <input.luz|input.lvl> [output.json]\n"
        "\n"
        "Converts a LUZ (zone) or LVL (scene/level) binary file to JSON.\n"
        "The format is auto-detected from the file extension.\n"
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

    nlohmann::json j;
    try {
        auto data = read_file(input);
        if (ext == ".luz") {
            j = lu::assets::luz_parse(data);
        } else if (ext == ".lvl") {
            j = lu::assets::lvl_parse(data);
        } else {
            std::fprintf(stderr, "Error: unknown extension '%s' (expected .luz or .lvl)\n", ext.c_str());
            return 1;
        }
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
