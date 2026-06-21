#pragma once

#include "zone_document.h"
#include "zone_tree_model.h"

#include <QWidget>
#include <QScrollArea>
#include <QFormLayout>
#include <QUndoStack>

namespace zone_editor {

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void set_undo_stack(QUndoStack* stack) { undo_stack_ = stack; }
    void set_client_context(const ClientContext* ctx) { ctx_ = ctx; }
    void show_node(ZoneTreeNode* node, ZoneDocument* doc);
    void clear();

signals:
    void data_changed();

private:
    void show_zone_properties(ZoneDocument* doc);
    void show_scene(ZoneTreeNode* node, ZoneDocument* doc);
    void show_environment(ZoneTreeNode* node, ZoneDocument* doc);
    void show_object(ZoneTreeNode* node, ZoneDocument* doc);
    void show_particle(ZoneTreeNode* node, ZoneDocument* doc);
    void show_boundary(ZoneTreeNode* node, ZoneDocument* doc);
    void show_transition(ZoneTreeNode* node, ZoneDocument* doc);
    void show_path(ZoneTreeNode* node, ZoneDocument* doc);
    void show_waypoint(ZoneTreeNode* node, ZoneDocument* doc);

    // Helpers to add editable rows
    void add_float(const QString& label, float value,
                   std::function<float&()> accessor, std::function<void()> dirty);
    void add_u32(const QString& label, uint32_t value,
                 std::function<uint32_t&()> accessor, std::function<void()> dirty);
    void add_s32(const QString& label, int32_t value,
                 std::function<int32_t&()> accessor, std::function<void()> dirty);
    void add_u64(const QString& label, uint64_t value,
                 std::function<uint64_t&()> accessor, std::function<void()> dirty);
    void add_string(const QString& label, const std::string& value,
                    std::function<std::string&()> accessor, std::function<void()> dirty);
    void add_bool(const QString& label, bool value,
                  std::function<bool&()> accessor, std::function<void()> dirty);
    void add_vec3(const QString& label, const lu::assets::Vec3& value,
                  std::function<lu::assets::Vec3&()> accessor, std::function<void()> dirty);
    void add_quat(const QString& label, const lu::assets::Quat& value,
                  std::function<lu::assets::Quat&()> accessor, std::function<void()> dirty);
    void add_readonly(const QString& label, const QString& value);
    void add_enum_u32(const QString& label, uint32_t value,
                      const QStringList& names,
                      std::function<uint32_t&()> accessor, std::function<void()> dirty);
    void add_lot(const QString& label, uint32_t lot,
                 std::function<uint32_t&()> accessor, std::function<void()> dirty);

    QScrollArea* scroll_;
    QWidget* form_widget_;
    QFormLayout* form_;
    QUndoStack* undo_stack_ = nullptr;
    const ClientContext* ctx_ = nullptr;
};

} // namespace zone_editor
