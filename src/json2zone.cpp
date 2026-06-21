#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/luz/luz_writer.h"
#include "netdevil/zone/lvl/lvl_json.h"
#include "netdevil/zone/lvl/lvl_writer.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: json2zone <input.json> <output.luz|output.lvl>\n");
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
        std::fprintf(stderr, "Error: cannot open %s\n", input.c_str());
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
        std::fprintf(stderr, "Error: cannot write %s\n", output.c_str());
        return 1;
    }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());

    std::fprintf(stderr, "Wrote %s (%zu bytes)\n", output.c_str(), data.size());
    return 0;
}
