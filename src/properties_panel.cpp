#include "properties_panel.h"
#include "zone_undo_commands.h"
#include "vec_edit_widget.h"
#include "ldf_table_widget.h"

using lu_widgets::VecEditWidget;
using lu_widgets::LdfTableWidget;

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace zone_editor {

PropertiesPanel::PropertiesPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    scroll_ = new QScrollArea;
    scroll_->setWidgetResizable(true);
    layout->addWidget(scroll_);

    clear();
}

void PropertiesPanel::clear() {
    form_widget_ = new QWidget;
    form_ = new QFormLayout(form_widget_);
    form_->setContentsMargins(8, 8, 8, 8);
    scroll_->setWidget(form_widget_);
}

void PropertiesPanel::add_readonly(const QString& label, const QString& value) {
    auto* edit = new QLineEdit(value);
    edit->setReadOnly(true);
    form_->addRow(label, edit);
}

void PropertiesPanel::add_float(const QString& label, float value,
                                 std::function<float&()> accessor, std::function<void()> dirty) {
    auto* spin = new QDoubleSpinBox;
    spin->setDecimals(4);
    spin->setRange(-1e6, 1e6);
    spin->setSingleStep(0.1);
    spin->setValue(value);
    form_->addRow(label, spin);

    connect(spin, &QDoubleSpinBox::editingFinished, this, [=, this]() {
        float new_val = static_cast<float>(spin->value());
        float old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditFloatCmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_u32(const QString& label, uint32_t value,
                               std::function<uint32_t&()> accessor, std::function<void()> dirty) {
    auto* spin = new QSpinBox;
    spin->setRange(0, INT_MAX);
    spin->setValue(static_cast<int>(value));
    form_->addRow(label, spin);

    connect(spin, &QSpinBox::editingFinished, this, [=, this]() {
        uint32_t new_val = static_cast<uint32_t>(spin->value());
        uint32_t old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditU32Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_s32(const QString& label, int32_t value,
                               std::function<int32_t&()> accessor, std::function<void()> dirty) {
    auto* spin = new QSpinBox;
    spin->setRange(INT_MIN, INT_MAX);
    spin->setValue(value);
    form_->addRow(label, spin);

    connect(spin, &QSpinBox::editingFinished, this, [=, this]() {
        int32_t new_val = static_cast<int32_t>(spin->value());
        int32_t old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditS32Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_u64(const QString& label, uint64_t value,
                               std::function<uint64_t&()> accessor, std::function<void()> dirty) {
    auto* edit = new QLineEdit(QString::number(value));
    form_->addRow(label, edit);

    connect(edit, &QLineEdit::editingFinished, this, [=, this]() {
        bool ok;
        uint64_t new_val = edit->text().toULongLong(&ok);
        if (!ok) return;
        uint64_t old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditU64Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_string(const QString& label, const std::string& value,
                                  std::function<std::string&()> accessor, std::function<void()> dirty) {
    auto* edit = new QLineEdit(QString::fromStdString(value));
    form_->addRow(label, edit);

    connect(edit, &QLineEdit::editingFinished, this, [=, this]() {
        std::string new_val = edit->text().toStdString();
        std::string old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditStringCmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_bool(const QString& label, bool value,
                                std::function<bool&()> accessor, std::function<void()> dirty) {
    auto* cb = new QCheckBox;
    cb->setChecked(value);
    form_->addRow(label, cb);

    connect(cb, &QCheckBox::toggled, this, [=, this](bool checked) {
        bool old_val = accessor();
        if (checked == old_val) return;
        accessor() = checked;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditBoolCmd(
                "Edit " + label, accessor, old_val, checked, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_vec3(const QString& label, const lu::assets::Vec3& value,
                                std::function<lu::assets::Vec3&()> accessor, std::function<void()> dirty) {
    auto* w = new VecEditWidget(3);
    w->set_vec3(value);
    form_->addRow(label, w);

    connect(w, &VecEditWidget::value_changed, this, [=, this]() {
        auto new_val = w->get_vec3();
        auto old_val = accessor();
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditVec3Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_quat(const QString& label, const lu::assets::Quat& value,
                                std::function<lu::assets::Quat&()> accessor, std::function<void()> dirty) {
    auto* w = new VecEditWidget(4);
    w->set_quat(value);
    form_->addRow(label, w);

    connect(w, &VecEditWidget::value_changed, this, [=, this]() {
        auto new_val = w->get_quat();
        auto old_val = accessor();
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditQuatCmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_enum_u32(const QString& label, uint32_t value,
                                    const QStringList& names,
                                    std::function<uint32_t&()> accessor, std::function<void()> dirty) {
    auto* combo = new QComboBox;
    for (int i = 0; i < names.size(); ++i) {
        combo->addItem(QString("%1 (%2)").arg(names[i]).arg(i), i);
    }
    int cur = static_cast<int>(value);
    if (cur >= 0 && cur < names.size()) {
        combo->setCurrentIndex(cur);
    } else {
        combo->addItem(QString("Unknown (%1)").arg(cur), cur);
        combo->setCurrentIndex(combo->count() - 1);
    }
    form_->addRow(label, combo);

    connect(combo, &QComboBox::currentIndexChanged, this, [=, this]() {
        uint32_t new_val = static_cast<uint32_t>(combo->currentData().toInt());
        uint32_t old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();
        if (undo_stack_) {
            undo_stack_->push(new EditU32Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

void PropertiesPanel::add_lot(const QString& label, uint32_t lot,
                               std::function<uint32_t&()> accessor, std::function<void()> dirty) {
    auto* spin = new QSpinBox;
    spin->setRange(0, INT_MAX);
    spin->setValue(static_cast<int>(lot));

    QString name_str;
    if (ctx_) {
        auto name = ctx_->lot_name(lot);
        if (!name.empty()) name_str = QString::fromStdString(name);
    }

    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(spin);
    auto* name_label = new QLabel(name_str);
    name_label->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(name_label);
    form_->addRow(label, widget);

    connect(spin, &QSpinBox::editingFinished, this, [=, this]() {
        uint32_t new_val = static_cast<uint32_t>(spin->value());
        uint32_t old_val = accessor();
        if (new_val == old_val) return;
        accessor() = new_val;
        if (dirty) dirty();

        // Update name label
        if (ctx_) {
            auto n = ctx_->lot_name(new_val);
            name_label->setText(n.empty() ? "" : QString::fromStdString(n));
        }

        if (undo_stack_) {
            undo_stack_->push(new EditU32Cmd(
                "Edit " + label, accessor, old_val, new_val, dirty,
                [this]() { emit data_changed(); }));
        }
        emit data_changed();
    });
}

// ── Node display implementations ────────────────────────────────────────────

void PropertiesPanel::show_node(ZoneTreeNode* node, ZoneDocument* doc) {
    if (!node || !doc) { clear(); return; }
    clear();

    switch (node->type) {
    case NodeType::ZoneProperties: show_zone_properties(doc); break;
    case NodeType::Scene: show_scene(node, doc); break;
    case NodeType::EnvironmentGroup: show_environment(node, doc); break;
    case NodeType::Object: show_object(node, doc); break;
    case NodeType::Particle: show_particle(node, doc); break;
    case NodeType::Boundary: show_boundary(node, doc); break;
    case NodeType::Transition: show_transition(node, doc); break;
    case NodeType::Path: show_path(node, doc); break;
    case NodeType::Waypoint: show_waypoint(node, doc); break;
    default:
        form_->addRow(new QLabel(QString::fromStdString(node->display_name)));
        break;
    }
}

void PropertiesPanel::show_zone_properties(ZoneDocument* doc) {
    auto dirty = [doc]() { doc->mark_luz_modified(); };

    add_readonly("Version", QString::number(doc->luz().version));
    add_readonly("File Revision", QString::number(doc->luz().file_revision));
    add_u32("World ID", doc->luz().world_id,
            [doc]() -> uint32_t& { return doc->luz().world_id; }, dirty);
    add_string("Raw Path", doc->luz().raw_path,
               [doc]() -> std::string& { return doc->luz().raw_path; }, dirty);
    add_string("Zone Name", doc->luz().zone_name,
               [doc]() -> std::string& { return doc->luz().zone_name; }, dirty);
    add_string("Zone Description", doc->luz().zone_description,
               [doc]() -> std::string& { return doc->luz().zone_description; }, dirty);
    add_vec3("Spawn Position", doc->luz().spawn_position,
             [doc]() -> lu::assets::Vec3& { return doc->luz().spawn_position; }, dirty);
    add_quat("Spawn Rotation", doc->luz().spawn_rotation,
             [doc]() -> lu::assets::Quat& { return doc->luz().spawn_rotation; }, dirty);
}

void PropertiesPanel::show_scene(ZoneTreeNode* node, ZoneDocument* doc) {
    int idx = node->index;
    auto dirty = [doc]() { doc->mark_luz_modified(); };

    add_string("Filename", doc->luz().scenes[idx].filename,
               [doc, idx]() -> std::string& { return doc->luz().scenes[idx].filename; }, dirty);
    add_u32("Scene ID", doc->luz().scenes[idx].id,
            [doc, idx]() -> uint32_t& { return doc->luz().scenes[idx].id; }, dirty);
    static const QStringList scene_types = {"General", "Audio"};
    add_enum_u32("Type", doc->luz().scenes[idx].type, scene_types,
                 [doc, idx]() -> uint32_t& { return doc->luz().scenes[idx].type; }, dirty);
    add_string("Name", doc->luz().scenes[idx].name,
               [doc, idx]() -> std::string& { return doc->luz().scenes[idx].name; }, dirty);
}

void PropertiesPanel::show_environment(ZoneTreeNode* node, ZoneDocument* doc) {
    int si = node->scene_index;
    if (!doc->has_scene(si)) return;
    auto dirty = [doc, si]() { doc->mark_scene_modified(si); };
    auto& env = doc->scene(si).environment;

    form_->addRow(new QLabel("<b>Lighting</b>"));
    add_float("Blend Time", env.lighting.blend_time,
              [doc, si]() -> float& { return doc->scene(si).environment.lighting.blend_time; }, dirty);
    add_vec3("Light Position", env.lighting.position,
             [doc, si]() -> lu::assets::Vec3& { return doc->scene(si).environment.lighting.position; }, dirty);

    form_->addRow(new QLabel("<b>Skydome</b>"));
    add_string("Filename", env.skydome.filename,
               [doc, si]() -> std::string& { return doc->scene(si).environment.skydome.filename; }, dirty);
    add_string("Sky Layer", env.skydome.sky_layer_filename,
               [doc, si]() -> std::string& { return doc->scene(si).environment.skydome.sky_layer_filename; }, dirty);
}

void PropertiesPanel::show_object(ZoneTreeNode* node, ZoneDocument* doc) {
    int si = node->scene_index;
    int oi = node->index;
    if (!doc->has_scene(si)) return;
    auto dirty = [doc, si]() { doc->mark_scene_modified(si); };
    auto& obj = doc->scene(si).objects[oi];

    add_u64("Object ID", obj.object_id,
            [doc, si, oi]() -> uint64_t& { return doc->scene(si).objects[oi].object_id; }, dirty);
    add_lot("LOT", obj.lot,
            [doc, si, oi]() -> uint32_t& { return doc->scene(si).objects[oi].lot; }, dirty);

    static const QStringList node_types = {
        "Environment", "Building", "Enemy", "NPC", "Rebuilder",
        "Spawned", "Cannon", "Bouncer", "Exhibit", "Moving Platform",
        "Springpad", "Sound", "Particle", "Placeholder", "Error Marker", "Player Start"
    };
    add_enum_u32("Node Type", static_cast<uint32_t>(obj.node_type), node_types,
                 [doc, si, oi]() -> uint32_t& {
                     return reinterpret_cast<uint32_t&>(doc->scene(si).objects[oi].node_type);
                 }, dirty);

    add_float("Scale", obj.scale,
              [doc, si, oi]() -> float& { return doc->scene(si).objects[oi].scale; }, dirty);
    add_vec3("Position", obj.position,
             [doc, si, oi]() -> lu::assets::Vec3& { return doc->scene(si).objects[oi].position; }, dirty);
    add_quat("Rotation", obj.rotation,
             [doc, si, oi]() -> lu::assets::Quat& { return doc->scene(si).objects[oi].rotation; }, dirty);

    form_->addRow(new QLabel("<b>Config</b>"));
    auto* ldf = new LdfTableWidget;
    ldf->set_entries(obj.config);
    form_->addRow(ldf);
}

void PropertiesPanel::show_particle(ZoneTreeNode* node, ZoneDocument* doc) {
    int si = node->scene_index;
    int pi = node->index;
    if (!doc->has_scene(si)) return;
    auto dirty = [doc, si]() { doc->mark_scene_modified(si); };
    auto& p = doc->scene(si).particles[pi];

    add_string("Effects", p.effect_names,
               [doc, si, pi]() -> std::string& { return doc->scene(si).particles[pi].effect_names; }, dirty);
    add_vec3("Position", p.position,
             [doc, si, pi]() -> lu::assets::Vec3& { return doc->scene(si).particles[pi].position; }, dirty);
    add_quat("Rotation", p.rotation,
             [doc, si, pi]() -> lu::assets::Quat& { return doc->scene(si).particles[pi].rotation; }, dirty);

    form_->addRow(new QLabel("<b>Config</b>"));
    auto* ldf = new LdfTableWidget;
    ldf->set_entries(p.config);
    form_->addRow(ldf);
}

void PropertiesPanel::show_boundary(ZoneTreeNode* node, ZoneDocument* doc) {
    int bi = node->index;
    auto dirty = [doc]() { doc->mark_luz_modified(); };

    add_vec3("Normal", doc->luz().boundaries[bi].normal,
             [doc, bi]() -> lu::assets::Vec3& { return doc->luz().boundaries[bi].normal; }, dirty);
    add_vec3("Point", doc->luz().boundaries[bi].point,
             [doc, bi]() -> lu::assets::Vec3& { return doc->luz().boundaries[bi].point; }, dirty);
    add_vec3("Spawn Location", doc->luz().boundaries[bi].spawn_location,
             [doc, bi]() -> lu::assets::Vec3& { return doc->luz().boundaries[bi].spawn_location; }, dirty);
}

void PropertiesPanel::show_transition(ZoneTreeNode* node, ZoneDocument* doc) {
    int ti = node->index;
    auto dirty = [doc]() { doc->mark_luz_modified(); };

    add_string("Name", doc->luz().transitions[ti].name,
               [doc, ti]() -> std::string& { return doc->luz().transitions[ti].name; }, dirty);
    add_float("Width", doc->luz().transitions[ti].width,
              [doc, ti]() -> float& { return doc->luz().transitions[ti].width; }, dirty);

    for (int p = 0; p < static_cast<int>(doc->luz().transitions[ti].points.size()); ++p) {
        form_->addRow(new QLabel(QString("<b>Point %1</b>").arg(p)));
        add_u32("Scene ID", doc->luz().transitions[ti].points[p].scene_id,
                [doc, ti, p]() -> uint32_t& { return doc->luz().transitions[ti].points[p].scene_id; }, dirty);
        add_u32("Layer ID", doc->luz().transitions[ti].points[p].layer_id,
                [doc, ti, p]() -> uint32_t& { return doc->luz().transitions[ti].points[p].layer_id; }, dirty);
        add_vec3("Position", doc->luz().transitions[ti].points[p].position,
                 [doc, ti, p]() -> lu::assets::Vec3& { return doc->luz().transitions[ti].points[p].position; }, dirty);
    }
}

void PropertiesPanel::show_path(ZoneTreeNode* node, ZoneDocument* doc) {
    int pi = node->index;
    auto dirty = [doc]() { doc->mark_luz_modified(); };
    auto& p = doc->luz().paths[pi];

    add_string("Name", p.name,
               [doc, pi]() -> std::string& { return doc->luz().paths[pi].name; }, dirty);

    static const QStringList path_types = {
        "NPC", "Moving Platform", "Property", "Camera",
        "Spawner", "Showcase", "Racing", "Rail"
    };
    add_enum_u32("Path Type", static_cast<uint32_t>(p.path_type), path_types,
                 [doc, pi]() -> uint32_t& {
                     return reinterpret_cast<uint32_t&>(doc->luz().paths[pi].path_type);
                 }, dirty);

    static const QStringList behaviors = {"Loop", "Bounce", "Once"};
    add_enum_u32("Behavior", static_cast<uint32_t>(p.behavior), behaviors,
                 [doc, pi]() -> uint32_t& {
                     return reinterpret_cast<uint32_t&>(doc->luz().paths[pi].behavior);
                 }, dirty);

    add_u32("Path Version", p.path_version,
            [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].path_version; }, dirty);
    add_u32("Flags", p.flags,
            [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].flags; }, dirty);

    if (p.path_type == lu::assets::LuzPathType::Spawner) {
        form_->addRow(new QLabel("<b>Spawner</b>"));
        add_lot("Spawned LOT", p.spawner.spawned_lot,
                [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].spawner.spawned_lot; }, dirty);
        add_u32("Respawn Time", p.spawner.respawn_time,
                [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].spawner.respawn_time; }, dirty);
        add_s32("Max To Spawn", p.spawner.max_to_spawn,
                [doc, pi]() -> int32_t& { return doc->luz().paths[pi].spawner.max_to_spawn; }, dirty);
        add_u32("Maintain", p.spawner.num_to_maintain,
                [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].spawner.num_to_maintain; }, dirty);
        add_u64("Spawner Object ID", p.spawner.spawner_object_id,
                [doc, pi]() -> uint64_t& { return doc->luz().paths[pi].spawner.spawner_object_id; }, dirty);
    } else if (p.path_type == lu::assets::LuzPathType::Property) {
        form_->addRow(new QLabel("<b>Property</b>"));
        add_u32("Price", p.property.price,
                [doc, pi]() -> uint32_t& { return doc->luz().paths[pi].property.price; }, dirty);
        add_string("Display Name", p.property.display_name,
                   [doc, pi]() -> std::string& { return doc->luz().paths[pi].property.display_name; }, dirty);
        add_string("Display Desc", p.property.display_desc,
                   [doc, pi]() -> std::string& { return doc->luz().paths[pi].property.display_desc; }, dirty);
        add_float("Max Build Height", p.property.max_build_height,
                  [doc, pi]() -> float& { return doc->luz().paths[pi].property.max_build_height; }, dirty);
    } else if (p.path_type == lu::assets::LuzPathType::Camera) {
        form_->addRow(new QLabel("<b>Camera</b>"));
        add_string("Next Path", p.camera.next_path,
                   [doc, pi]() -> std::string& { return doc->luz().paths[pi].camera.next_path; }, dirty);
    } else if (p.path_type == lu::assets::LuzPathType::MovingPlatform) {
        form_->addRow(new QLabel("<b>Platform</b>"));
        add_bool("Time Based", p.platform.time_based_movement,
                 [doc, pi]() -> bool& { return doc->luz().paths[pi].platform.time_based_movement; }, dirty);
    }
}

void PropertiesPanel::show_waypoint(ZoneTreeNode* node, ZoneDocument* doc) {
    int pi = node->path_index;
    int wi = node->index;
    auto dirty = [doc]() { doc->mark_luz_modified(); };

    add_vec3("Position", doc->luz().paths[pi].waypoints[wi].position,
             [doc, pi, wi]() -> lu::assets::Vec3& { return doc->luz().paths[pi].waypoints[wi].position; }, dirty);
    add_quat("Rotation", doc->luz().paths[pi].waypoints[wi].rotation,
             [doc, pi, wi]() -> lu::assets::Quat& { return doc->luz().paths[pi].waypoints[wi].rotation; }, dirty);
    add_float("Speed", doc->luz().paths[pi].waypoints[wi].platform_speed,
              [doc, pi, wi]() -> float& { return doc->luz().paths[pi].waypoints[wi].platform_speed; }, dirty);
    add_float("Wait", doc->luz().paths[pi].waypoints[wi].platform_wait,
              [doc, pi, wi]() -> float& { return doc->luz().paths[pi].waypoints[wi].platform_wait; }, dirty);

    {
        form_->addRow(new QLabel("<b>Config</b>"));
        auto* ldf = new LdfTableWidget;
        ldf->set_config(doc->luz().paths[pi].waypoints[wi].config);
        form_->addRow(ldf);
    }
}

} // namespace zone_editor
