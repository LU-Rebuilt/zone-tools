#pragma once

#include "client_context.h"

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>

namespace zone_editor {

class ZoneListPanel : public QWidget {
    Q_OBJECT
public:
    explicit ZoneListPanel(QWidget* parent = nullptr);

    void set_context(const ClientContext* ctx);

signals:
    void zone_selected(int zone_id, const QString& luz_path);

private slots:
    void on_search_changed(const QString& text);
    void on_item_clicked(QListWidgetItem* item);

private:
    void populate();

    const ClientContext* ctx_ = nullptr;
    QLineEdit* search_box_;
    QListWidget* list_;
};

} // namespace zone_editor
