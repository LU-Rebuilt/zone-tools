#pragma once

#include "client_context.h"
#include "zone_document.h"
#include "zone_tree_model.h"
#include "zone_list_panel.h"
#include "properties_panel.h"

#include <QMainWindow>
#include <QTreeView>
#include <QSettings>
#include <QUndoStack>

namespace zone_editor {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void open_client_dir(const std::string& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_open_client();
    void on_save();
    void on_save_as();
    void on_zone_selected(int zone_id, const QString& luz_path);
    void on_tree_selection_changed();
    void on_tree_context_menu(const QPoint& pos);
    void on_export_json();
    void on_import_json();

private:
    void update_title();

    ClientContext client_;
    ZoneDocument doc_;

    ZoneListPanel* zone_list_;
    ZoneTreeModel* tree_model_;
    QTreeView* tree_view_;
    PropertiesPanel* properties_;

    QUndoStack* undo_stack_;
};

} // namespace zone_editor
