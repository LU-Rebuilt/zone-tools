cmake_minimum_required(VERSION 3.25)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
project(zone-tools VERSION 0.1.0 LANGUAGES C CXX DESCRIPTION "LEGO Universe LUZ/LVL zone tools")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(lu_assets
    GIT_REPOSITORY https://github.com/LU-Rebuilt/lu-assets.git
    GIT_TAG main
)
FetchContent_MakeAvailable(lu_assets)

# ── CLI tools (no Qt dependency) ──────────────────────────────────────────────

add_executable(zone2json src/zone2json.cpp)
target_link_libraries(zone2json PRIVATE lu_assets)

add_executable(json2zone src/json2zone.cpp)
target_link_libraries(json2zone PRIVATE lu_assets)

# ── GUI editor (Qt6) ─────────────────────────────────────────────────────────

find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)

    FetchContent_Declare(tool_common
        GIT_REPOSITORY https://github.com/LU-Rebuilt/tool-common.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(tool_common)

    add_executable(zone_editor
        src/main.cpp
        src/main_window.h src/main_window.cpp
        src/zone_document.h src/zone_document.cpp
        src/client_context.h src/client_context.cpp
        src/zone_list_panel.h src/zone_list_panel.cpp
        src/zone_tree_model.h src/zone_tree_model.cpp
        src/properties_panel.h src/properties_panel.cpp
        src/zone_undo_commands.h src/zone_undo_commands.cpp
    )

    target_link_libraries(zone_editor PRIVATE
        Qt6::Widgets
        lu_assets
        qt_common
        lu_widgets
    )
    target_include_directories(zone_editor PRIVATE src)

    # Tests
    find_package(Qt6 COMPONENTS Test QUIET)
    if(Qt6Test_FOUND)
        enable_testing()
        add_executable(zone_tools_tests
            tests/test_zone_editor.cpp
            src/client_context.h src/client_context.cpp
            src/zone_document.h src/zone_document.cpp
            src/zone_tree_model.h src/zone_tree_model.cpp
            src/zone_undo_commands.h src/zone_undo_commands.cpp
        )
        target_link_libraries(zone_tools_tests PRIVATE
            Qt6::Widgets Qt6::Test
            lu_assets
            lu_widgets
        )
        target_include_directories(zone_tools_tests PRIVATE src)
        add_test(NAME zone_tools_tests COMMAND zone_tools_tests)
    endif()
else()
    message(STATUS "Qt6 not found — building CLI tools only (zone2json, json2zone)")
endif()
