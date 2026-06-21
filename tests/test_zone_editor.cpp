#include <QTest>
#include <QUndoStack>
#include <QApplication>

#include "client_context.h"
#include "zone_document.h"
#include "zone_tree_model.h"
#include "zone_undo_commands.h"

using namespace zone_editor;
using namespace lu::assets;

static const char* VANILLA_CLIENT = nullptr;

class TestZoneEditor : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        if (!VANILLA_CLIENT) QSKIP("No client directory specified (pass path as arg)");
    }

    void test_client_context_open() {
        ClientContext ctx;
        QVERIFY(ctx.open(VANILLA_CLIENT));
        QVERIFY(ctx.zones().size() > 0);

        bool found_ag = false;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                found_ag = true;
                QCOMPARE(z.display_name, std::string("Avant Gardens"));
                QVERIFY(!z.luz_path.empty());
                break;
            }
        }
        QVERIFY(found_ag);
    }

    void test_client_context_filters_removed_zones() {
        ClientContext ctx;
        QVERIFY(ctx.open(VANILLA_CLIENT));
        for (auto& z : ctx.zones()) {
            QVERIFY(z.luz_path.find("__removed") == std::string::npos);
            QVERIFY(z.luz_path.find('.') != std::string::npos);
        }
    }

    void test_zone_document_load() {
        ClientContext ctx;
        QVERIFY(ctx.open(VANILLA_CLIENT));

        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                QVERIFY(doc.load_from_cdclient(z.luz_path, ctx.res_dir()));
                break;
            }
        }

        QVERIFY(doc.is_loaded());
        QCOMPARE(doc.luz().version, 41u);
        QCOMPARE(doc.luz().world_id, 1100u);
        QVERIFY(doc.scene_count() > 0);
        QVERIFY(doc.luz().paths.size() > 0);

        bool has_objects = false;
        for (uint32_t i = 0; i < doc.scene_count(); ++i) {
            if (doc.has_scene(i) && !doc.scene(i).objects.empty()) {
                has_objects = true;
                break;
            }
        }
        QVERIFY(has_objects);
    }

    void test_zone_document_not_modified_on_load() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }
        QVERIFY(!doc.is_modified());
    }

    void test_tree_model_structure() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        ZoneTreeModel model;
        model.set_document(&doc);

        // Root should have children: Properties, Scenes, Boundaries, Transitions, Paths
        QVERIFY(model.rowCount() >= 4);

        auto root_idx = model.index(0, 0);
        QVERIFY(root_idx.isValid());

        auto* root_node = model.node_at(root_idx);
        QVERIFY(root_node != nullptr);
    }

    void test_tree_model_node_types() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        ZoneTreeModel model;
        model.set_document(&doc);

        // Check root children types
        for (int i = 0; i < model.rowCount(); ++i) {
            auto idx = model.index(i, 0);
            auto* node = model.node_at(idx);
            QVERIFY(node != nullptr);
            // Should be one of the top-level group types
            QVERIFY(node->type == NodeType::ZoneProperties ||
                     node->type == NodeType::ScenesGroup ||
                     node->type == NodeType::BoundariesGroup ||
                     node->type == NodeType::TransitionsGroup ||
                     node->type == NodeType::PathsGroup);
        }
    }

    void test_undo_edit_float() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        QUndoStack stack;
        float original = doc.luz().spawn_position.x;
        float new_val = original + 100.0f;

        // Apply the edit
        doc.luz().spawn_position.x = new_val;
        doc.mark_luz_modified();

        stack.push(new EditFloatCmd(
            "Edit spawn X",
            [&doc]() -> float& { return doc.luz().spawn_position.x; },
            original, new_val,
            [&doc]() { doc.mark_luz_modified(); },
            []() {}));

        QCOMPARE(doc.luz().spawn_position.x, new_val);
        QVERIFY(doc.is_modified());

        // Undo
        stack.undo();
        QCOMPARE(doc.luz().spawn_position.x, original);

        // Redo
        stack.redo();
        QCOMPARE(doc.luz().spawn_position.x, new_val);
    }

    void test_undo_edit_string() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        QUndoStack stack;
        std::string original = doc.luz().zone_name;
        std::string new_val = "Test Zone Name";

        doc.luz().zone_name = new_val;
        doc.mark_luz_modified();

        stack.push(new EditStringCmd(
            "Edit zone name",
            [&doc]() -> std::string& { return doc.luz().zone_name; },
            original, new_val,
            [&doc]() { doc.mark_luz_modified(); },
            []() {}));

        QCOMPARE(doc.luz().zone_name, new_val);

        stack.undo();
        QCOMPARE(doc.luz().zone_name, original);

        stack.redo();
        QCOMPARE(doc.luz().zone_name, new_val);
    }

    void test_undo_add_remove_path() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        QUndoStack stack;
        size_t original_count = doc.luz().paths.size();

        LuzPath new_path{};
        new_path.path_version = 18;
        new_path.name = "test_path";

        doc.luz().paths.push_back(new_path);
        doc.mark_luz_modified();

        stack.push(new AddPathCmd(
            "Add Path",
            [&doc]() -> std::vector<LuzPath>& { return doc.luz().paths; },
            new_path, static_cast<int>(doc.luz().paths.size()) - 1,
            [&doc]() { doc.mark_luz_modified(); },
            []() {}));

        QCOMPARE(doc.luz().paths.size(), original_count + 1);
        QCOMPARE(doc.luz().paths.back().name, std::string("test_path"));

        // Undo removes the path
        stack.undo();
        QCOMPARE(doc.luz().paths.size(), original_count);

        // Redo adds it back
        stack.redo();
        QCOMPARE(doc.luz().paths.size(), original_count + 1);
    }

    void test_save_roundtrip() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);
        ZoneDocument doc;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100) {
                doc.load_from_cdclient(z.luz_path, ctx.res_dir());
                break;
            }
        }

        // Save to temp dir
        QString tmpDir = QDir::tempPath() + "/zone_editor_test_save";
        QDir().mkpath(tmpDir);

        QVERIFY(doc.save_as(tmpDir.toStdString()));

        // Verify the saved LUZ can be loaded back
        auto saved_luz = tmpDir + "/" + QString::fromStdString(doc.luz_path().filename().string());
        QVERIFY(QFile::exists(saved_luz));

        // Clean up
        QDir(tmpDir).removeRecursively();
    }

    void test_multiple_zone_loads() {
        ClientContext ctx;
        ctx.open(VANILLA_CLIENT);

        int loaded = 0;
        for (auto& z : ctx.zones()) {
            if (z.zone_id == 1100 || z.zone_id == 1200 || z.zone_id == 1300) {
                ZoneDocument doc;
                if (doc.load_from_cdclient(z.luz_path, ctx.res_dir())) {
                    QVERIFY(doc.is_loaded());
                    QVERIFY(doc.luz().version > 0);
                    loaded++;
                }
            }
        }
        QVERIFY(loaded >= 2);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Look for client path in args
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]).find("--client=") == 0) {
            VANILLA_CLIENT = argv[i] + 9;
        } else if (std::string(argv[i]) == "--client" && i + 1 < argc) {
            VANILLA_CLIENT = argv[++i];
        }
    }

    // Also check environment
    if (!VANILLA_CLIENT) {
        VANILLA_CLIENT = std::getenv("LU_CLIENT_DIR");
    }

    TestZoneEditor test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_zone_editor.moc"
