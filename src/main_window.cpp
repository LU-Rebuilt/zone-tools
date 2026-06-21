#include "main_window.h"
#include "zone_undo_commands.h"
#include "file_browser.h"

#include "netdevil/zone/luz/luz_json.h"
#include "netdevil/zone/lvl/lvl_json.h"

#include <QMenuBar>
#include <QMenu>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QUndoView>

#include <fstream>
#include <QCloseEvent>

namespace zone_editor {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("LU Zone Editor");
    resize(1400, 800);

    // Undo stack
    undo_stack_ = new QUndoStack(this);

    // Menu bar
    auto* file_menu = menuBar()->addMenu("&File");
    file_menu->addAction("&Open Client...", Qt::CTRL | Qt::Key_O, this, &MainWindow::on_open_client);
    file_menu->addSeparator();
    file_menu->addAction("&Save", Qt::CTRL | Qt::Key_S, this, &MainWindow::on_save);
    file_menu->addAction("Save &As...", Qt::CTRL | Qt::SHIFT | Qt::Key_S, this, &MainWindow::on_save_as);
    file_menu->addSeparator();
    file_menu->addAction("&Export to JSON...", Qt::CTRL | Qt::Key_E, this, &MainWindow::on_export_json);
    file_menu->addAction("&Import from JSON...", Qt::CTRL | Qt::Key_I, this, &MainWindow::on_import_json);
    file_menu->addSeparator();
    file_menu->addAction("&Quit", Qt::CTRL | Qt::Key_Q, this, &QWidget::close);

    auto* edit_menu = menuBar()->addMenu("&Edit");
    auto* undo_action = undo_stack_->createUndoAction(this, "&Undo");
    undo_action->setShortcut(QKeySequence::Undo);
    edit_menu->addAction(undo_action);
    auto* redo_action = undo_stack_->createRedoAction(this, "&Redo");
    redo_action->setShortcut(QKeySequence::Redo);
    edit_menu->addAction(redo_action);
    edit_menu->addSeparator();

    // Edit history dock
    undo_view_ = new QUndoView(undo_stack_);
    undo_view_->setEmptyLabel("<no changes>");
    undo_dock_ = new QDockWidget("Edit History", this);
    undo_dock_->setWidget(undo_view_);
    addDockWidget(Qt::RightDockWidgetArea, undo_dock_);
    edit_menu->addAction(undo_dock_->toggleViewAction());

    // Layout: 3-way horizontal splitter
    auto* splitter = new QSplitter(Qt::Horizontal);
    setCentralWidget(splitter);

    // Left: zone list
    zone_list_ = new ZoneListPanel;
    splitter->addWidget(zone_list_);

    // Center: tree view
    tree_model_ = new ZoneTreeModel(this);
    tree_view_ = new QTreeView;
    tree_view_->setModel(tree_model_);
    tree_view_->setHeaderHidden(false);
    tree_view_->setAlternatingRowColors(true);
    tree_view_->setContextMenuPolicy(Qt::CustomContextMenu);
    splitter->addWidget(tree_view_);

    // Right: properties
    properties_ = new PropertiesPanel;
    properties_->set_undo_stack(undo_stack_);
    splitter->addWidget(properties_);

    splitter->setSizes({250, 600, 400});

    // Connections
    connect(zone_list_, &ZoneListPanel::zone_selected, this, &MainWindow::on_zone_selected);
    connect(tree_view_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::on_tree_selection_changed);
    connect(properties_, &PropertiesPanel::data_changed, this, [this]() {
        tree_model_->rebuild();
        update_title();
    });
    connect(tree_view_, &QTreeView::customContextMenuRequested, this, &MainWindow::on_tree_context_menu);

    statusBar()->showMessage("Open a client directory to get started.");
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (doc_.is_modified()) {
        auto result = QMessageBox::question(this, "Unsaved Changes",
            "There are unsaved changes. Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Save) {
            on_save();
            event->accept();
        } else if (result == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void MainWindow::open_client_dir(const std::string& path) {
    if (!client_.open(path)) {
        QMessageBox::critical(this, "Error",
            QString("Failed to open client at %1.\n"
                    "Make sure the directory contains res/cdclient.sqlite")
            .arg(QString::fromStdString(path)));
        return;
    }

    QSettings settings;
    settings.setValue("last_client_dir", QString::fromStdString(path));

    zone_list_->set_context(&client_);
    tree_model_->set_document(nullptr);
    properties_->set_client_context(&client_);
    properties_->clear();

    statusBar()->showMessage(
        QString("Loaded %1 zones from %2")
        .arg(client_.zones().size())
        .arg(QString::fromStdString(path)));

    update_title();
}

void MainWindow::on_open_client() {
    QSettings settings;
    QString last = settings.value("last_client_dir").toString();

    QString dir = QFileDialog::getExistingDirectory(this, "Open Client Directory", last);
    if (dir.isEmpty()) return;

    open_client_dir(dir.toStdString());
}

void MainWindow::on_save() {
    if (!doc_.is_loaded()) return;
    if (doc_.save()) {
        statusBar()->showMessage("Saved.");
        update_title();
    } else {
        QMessageBox::critical(this, "Error", "Failed to save zone files.");
    }
}

void MainWindow::on_save_as() {
    if (!doc_.is_loaded()) return;
    QString dir = QFileDialog::getExistingDirectory(this, "Save Zone To Directory");
    if (dir.isEmpty()) return;
    if (doc_.save_as(dir.toStdString())) {
        statusBar()->showMessage("Saved to " + dir);
    } else {
        QMessageBox::critical(this, "Error", "Failed to save zone files.");
    }
}

void MainWindow::on_zone_selected(int zone_id, const QString& luz_path) {
    undo_stack_->clear();

    if (!doc_.load_from_cdclient(luz_path.toStdString(), client_.res_dir())) {
        QMessageBox::critical(this, "Error",
            QString("Failed to load zone %1").arg(zone_id));
        return;
    }

    tree_model_->set_document(&doc_, &client_);
    properties_->clear();

    tree_view_->expandToDepth(1);

    statusBar()->showMessage(
        QString("Zone %1: %2 scenes, %3 paths")
        .arg(zone_id)
        .arg(doc_.scene_count())
        .arg(doc_.luz().paths.size()));

    update_title();
}

void MainWindow::on_tree_selection_changed() {
    auto idx = tree_view_->currentIndex();
    if (!idx.isValid()) {
        properties_->clear();
        return;
    }
    auto* node = tree_model_->node_at(idx);
    properties_->show_node(node, &doc_);
}

void MainWindow::update_title() {
    QString title = "LU Zone Editor";
    if (doc_.is_loaded()) {
        title += " - " + QString::fromStdString(doc_.luz_path().filename().string());
        if (doc_.is_modified()) title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::on_export_json() {
    if (!doc_.is_loaded()) return;

    QString path = QFileDialog::getSaveFileName(this, "Export Zone to JSON", {},
        "JSON Files (*.json);;All Files (*)");
    if (path.isEmpty()) return;

    try {
        nlohmann::json j;
        j["luz"] = doc_.luz();

        nlohmann::json scenes_json = nlohmann::json::object();
        for (uint32_t i = 0; i < doc_.scene_count(); ++i) {
            if (doc_.has_scene(i)) {
                scenes_json[std::to_string(i)] = doc_.scene(i);
            }
        }
        j["scenes"] = scenes_json;

        std::ofstream out(path.toStdString());
        if (!out) throw std::runtime_error("cannot write file");
        out << j.dump(2) << '\n';

        statusBar()->showMessage("Exported to " + path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Export Error", e.what());
    }
}

void MainWindow::on_import_json() {
    if (!doc_.is_loaded()) {
        QMessageBox::information(this, "Import", "Load a zone first, then import JSON to overwrite it.");
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "Import Zone from JSON", {},
        "JSON Files (*.json);;All Files (*)");
    if (path.isEmpty()) return;

    try {
        std::ifstream in(path.toStdString());
        if (!in) throw std::runtime_error("cannot open file");

        nlohmann::json j;
        in >> j;

        if (j.contains("luz")) {
            doc_.luz() = j.at("luz").get<lu::assets::LuzFile>();
            doc_.mark_luz_modified();
        }

        if (j.contains("scenes")) {
            for (auto& [key, val] : j.at("scenes").items()) {
                uint32_t idx = std::stoul(key);
                if (doc_.has_scene(idx)) {
                    doc_.scene(idx) = val.get<lu::assets::LvlFile>();
                    doc_.mark_scene_modified(idx);
                }
            }
        }

        undo_stack_->clear();
        tree_model_->set_document(&doc_);
        properties_->clear();
        update_title();
        statusBar()->showMessage("Imported from " + path);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Import Error", e.what());
    }
}

void MainWindow::on_tree_context_menu(const QPoint& pos) {
    auto idx = tree_view_->indexAt(pos);
    if (!idx.isValid() || !doc_.is_loaded()) return;

    auto* node = tree_model_->node_at(idx);
    if (!node) return;

    auto refresh = [this]() {
        tree_model_->rebuild();
        properties_->clear();
        update_title();
    };

    QMenu menu;

    if (node->type == NodeType::ObjectsGroup && node->scene_index >= 0) {
        int si = node->scene_index;
        menu.addAction("Add Object", [=, this]() {
            lu::assets::LvlObject obj{};
            obj.lot = 1;
            obj.scale = 1.0f;
            auto& objs = doc_.scene(si).objects;
            objs.push_back(obj);
            doc_.mark_scene_modified(si);
            undo_stack_->push(new AddObjectCmd(
                "Add Object",
                [this, si]() -> std::vector<lu::assets::LvlObject>& { return doc_.scene(si).objects; },
                obj, static_cast<int>(objs.size()) - 1,
                [this, si]() { doc_.mark_scene_modified(si); }, refresh));
            refresh();
        });
    }

    if (node->type == NodeType::Object && node->scene_index >= 0) {
        int si = node->scene_index;
        int oi = node->index;
        menu.addAction("Remove Object", [=, this]() {
            undo_stack_->push(new RemoveObjectCmd(
                "Remove Object",
                [this, si]() -> std::vector<lu::assets::LvlObject>& { return doc_.scene(si).objects; },
                oi,
                [this, si]() { doc_.mark_scene_modified(si); }, refresh));
            doc_.scene(si).objects.erase(doc_.scene(si).objects.begin() + oi);
            doc_.mark_scene_modified(si);
            refresh();
        });
    }

    if (node->type == NodeType::PathsGroup) {
        menu.addAction("Add Path", [=, this]() {
            lu::assets::LuzPath path{};
            path.path_version = 18;
            path.name = "new_path";
            auto& paths = doc_.luz().paths;
            paths.push_back(path);
            doc_.mark_luz_modified();
            undo_stack_->push(new AddPathCmd(
                "Add Path",
                [this]() -> std::vector<lu::assets::LuzPath>& { return doc_.luz().paths; },
                path, static_cast<int>(paths.size()) - 1,
                [this]() { doc_.mark_luz_modified(); }, refresh));
            refresh();
        });
    }

    if (node->type == NodeType::Path) {
        int pi = node->index;
        menu.addAction("Remove Path", [=, this]() {
            undo_stack_->push(new RemovePathCmd(
                "Remove Path",
                [this]() -> std::vector<lu::assets::LuzPath>& { return doc_.luz().paths; },
                pi,
                [this]() { doc_.mark_luz_modified(); }, refresh));
            doc_.luz().paths.erase(doc_.luz().paths.begin() + pi);
            doc_.mark_luz_modified();
            refresh();
        });
    }

    if (node->type == NodeType::WaypointsGroup && node->path_index >= 0) {
        int pi = node->path_index;
        menu.addAction("Add Waypoint", [=, this]() {
            lu::assets::LuzWaypoint wp{};
            auto& wps = doc_.luz().paths[pi].waypoints;
            wps.push_back(wp);
            doc_.mark_luz_modified();
            undo_stack_->push(new AddWaypointCmd(
                "Add Waypoint",
                [this, pi]() -> std::vector<lu::assets::LuzWaypoint>& { return doc_.luz().paths[pi].waypoints; },
                wp, static_cast<int>(wps.size()) - 1,
                [this]() { doc_.mark_luz_modified(); }, refresh));
            refresh();
        });
    }

    if (node->type == NodeType::Waypoint && node->path_index >= 0) {
        int pi = node->path_index;
        int wi = node->index;
        menu.addAction("Remove Waypoint", [=, this]() {
            undo_stack_->push(new RemoveWaypointCmd(
                "Remove Waypoint",
                [this, pi]() -> std::vector<lu::assets::LuzWaypoint>& { return doc_.luz().paths[pi].waypoints; },
                wi,
                [this]() { doc_.mark_luz_modified(); }, refresh));
            doc_.luz().paths[pi].waypoints.erase(doc_.luz().paths[pi].waypoints.begin() + wi);
            doc_.mark_luz_modified();
            refresh();
        });
    }

    if (!menu.isEmpty()) {
        menu.exec(tree_view_->viewport()->mapToGlobal(pos));
    }
}

} // namespace zone_editor
