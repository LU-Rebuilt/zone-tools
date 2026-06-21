#include "zone_list_panel.h"

#include <QVBoxLayout>

namespace zone_editor {

ZoneListPanel::ZoneListPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    search_box_ = new QLineEdit;
    search_box_->setPlaceholderText("Search zones...");
    layout->addWidget(search_box_);

    list_ = new QListWidget;
    layout->addWidget(list_);

    connect(search_box_, &QLineEdit::textChanged, this, &ZoneListPanel::on_search_changed);
    connect(list_, &QListWidget::itemClicked, this, &ZoneListPanel::on_item_clicked);
}

void ZoneListPanel::set_context(const ClientContext* ctx) {
    ctx_ = ctx;
    populate();
}

void ZoneListPanel::populate() {
    list_->clear();
    if (!ctx_) return;

    QString filter = search_box_->text().toLower();
    for (auto& z : ctx_->zones()) {
        QString name = QString::fromStdString(z.display_name);
        QString label = QString("[%1] %2").arg(z.zone_id).arg(name);

        if (!filter.isEmpty() && !label.toLower().contains(filter)) continue;

        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, z.zone_id);
        item->setData(Qt::UserRole + 1, QString::fromStdString(z.luz_path));
        list_->addItem(item);
    }
}

void ZoneListPanel::on_search_changed(const QString&) {
    populate();
}

void ZoneListPanel::on_item_clicked(QListWidgetItem* item) {
    int zone_id = item->data(Qt::UserRole).toInt();
    QString luz_path = item->data(Qt::UserRole + 1).toString();
    emit zone_selected(zone_id, luz_path);
}

} // namespace zone_editor
