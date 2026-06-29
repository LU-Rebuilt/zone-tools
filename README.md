# zone-tools

CLI and GUI tools for working with LEGO Universe zone files (LUZ/LVL).

## Tools

### zone2json / json2zone (CLI)

Convert a full zone (LUZ + all referenced LVL scenes) to and from a single JSON file.

```bash
# Zone → JSON (reads LUZ and all LVL scenes automatically)
zone2json <input.luz> [output.json]
zone2json nd_avant_gardens.luz                    # outputs nd_avant_gardens.json

# JSON → Zone (writes LUZ + all LVL scenes into output directory)
json2zone <input.json> <output_dir>
json2zone nd_avant_gardens.json output/           # writes LUZ + LVLs into output/
```

`zone2json` resolves scene LVL paths relative to the LUZ file with case-insensitive matching (works on both Linux and Windows). `json2zone` recreates the original directory structure.

Both tools support `-h`/`--help`. Byte-identical round-trips verified across all client versions (0.179.12 through 1.10.64): 348 LUZ files and 2003 CHNK-format LVL files with zero failures. Non-CHNK LVL files (LUP property levels, alpha/beta inline format) and non-standard LUZ containers (some beta clients) are not yet supported.

### zone_editor (GUI)

Qt6-based visual editor for LUZ/LVL zone data.

```bash
zone_editor [client_dir]
```

Pass a client directory on the command line, or use File → Open Client. The last-used directory is remembered between sessions.

**Client loading:**
- Reads CDClient database (FDB is auto-converted to SQLite via lu-assets) and locale XML for zone/object display names
- Falls back to scanning for `.luz` files if no database is found
- Case-insensitive path resolution for cross-platform compatibility

**Zone browser:**
- Searchable zone list with localized display names (e.g. "Avant Gardens")
- Filters as you type

**Zone hierarchy tree:**
- Zone properties (world ID, spawn position/rotation, name, description)
- Scenes with environment editing (lighting blend time/position, skydome filenames)
- Objects with LOT name lookup, node type enum, position/rotation/scale, LDF config table
- Particles with effect names, position/rotation, LDF config table
- Paths with type-specific editors:
  - Spawner: spawned LOT, respawn time, max/maintain counts, spawner object ID
  - Property: price, display name/description, max build height
  - Camera: next path
  - Moving Platform: time-based movement flag
- Waypoints with position, rotation, speed, wait, and key-value config table
- Boundaries (normal, point, spawn location)
- Transitions (name, width, per-point scene/layer IDs and positions)

**Editing:**
- Type-appropriate property widgets (spin boxes, combo boxes for enums, LOT name lookup, Vec3/Quat editors, checkboxes)
- LDF config table editor with add/remove and type selection
- Full undo/redo with dockable Edit History panel
- Context menu add/remove for objects, paths, waypoints
- Unsaved-changes prompt on close

**File operations:**
- Save / Save As to binary (LUZ + all modified LVL scenes)
- Export entire zone (LUZ + all scenes) to a single JSON file
- Import JSON to overwrite the currently loaded zone

## Building

Requires CMake 3.25+ and a C++20 compiler. Qt6 is optional — without it, only the CLI tools are built.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Dependencies are fetched automatically via CMake FetchContent:
- [lu-assets](https://github.com/LU-Rebuilt/lu-assets) — LUZ/LVL readers, writers, JSON serialization, CDClient FDB→SQLite conversion
- [tool-common](https://github.com/LU-Rebuilt/tool-common) — shared Qt widgets (file browser, LDF table, Vec3/Quat editors)

## Testing

```bash
LU_CLIENT_DIR=/path/to/client QT_QPA_PLATFORM=offscreen ./build/zone_tools_tests
```

## License

See [LICENSE](LICENSE).
