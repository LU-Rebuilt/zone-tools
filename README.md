# zone-tools

CLI and GUI tools for working with LEGO Universe zone files (LUZ/LVL).

## Tools

### zone2json / json2zone (CLI)

Convert between LUZ/LVL binary and JSON. Auto-detects format by file extension.

```bash
# LUZ/LVL → JSON
zone2json nd_avant_gardens.luz    # outputs nd_avant_gardens.json
zone2json nd_avant_gardens.lvl    # outputs nd_avant_gardens.json

# JSON → LUZ/LVL
json2zone zone.json output.luz
json2zone scene.json output.lvl
```

Byte-identical round-trips verified on all shipped client files.

### zone_editor (GUI)

Qt6-based visual editor for LUZ/LVL zone data.

- Point at a client directory — reads CDClient (FDB or SQLite) and locale for zone names
- Browse all zones with localized display names (e.g. "Avant Gardens")
- Full zone hierarchy tree: scenes, objects, particles, paths, waypoints, boundaries, transitions
- Editable properties with type-appropriate widgets (combo boxes for enums, LOT name lookup, Vec3/Quat editors)
- LDF config table editor with add/remove and type selection dropdown
- Full undo/redo history
- Context menu add/remove for objects, paths, waypoints
- Save / Save As to binary, Export / Import JSON
- Works with any client directory, including those with only FDB (no SQLite)

## Building

Requires CMake 3.25+ and a C++20 compiler. Qt6 is optional — without it, only the CLI tools are built.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Dependencies are fetched automatically via CMake FetchContent:
- [lu-assets](https://github.com/LU-Rebuilt/lu-assets) — LUZ/LVL readers, writers, JSON serialization
- [tool-common](https://github.com/LU-Rebuilt/tool-common) — shared Qt widgets (file browser, LDF table, Vec3/Quat editors)

## Testing

```bash
LU_CLIENT_DIR=/path/to/client QT_QPA_PLATFORM=offscreen ./build/zone_tools_tests
```

## License

See [LICENSE](LICENSE).
