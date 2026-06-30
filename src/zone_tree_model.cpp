#include "zone_tree_model.h"

#include <sstream>

namespace zone_editor {

static std::string fmt_vec3(const lu::assets::Vec3& v) {
    std::ostringstream s;
    s.precision(2);
    s << std::fixed << v.x << ", " << v.y << ", " << v.z;
    return s.str();
}

static std::string ldf_search_str(const std::vector<lu::assets::LdfEntry>& config) {
    std::string out;
    for (auto& e : config) {
        out += ' ';
        out += e.key;
        out += '=';
        out += e.raw_value;
    }
    return out;
}

static std::unique_ptr<ZoneTreeNode> make_node(NodeType type, const std::string& name,
                                                const std::string& summary = {},
                                                int index = -1) {
    auto n = std::make_unique<ZoneTreeNode>();
    n->type = type;
    n->display_name = name;
    n->summary = summary;
    n->index = index;
    return n;
}

static void add_child(ZoneTreeNode* parent, std::unique_ptr<ZoneTreeNode> child) {
    child->parent = parent;
    child->row_in_parent = static_cast<int>(parent->children.size());
    parent->children.push_back(std::move(child));
}

ZoneTreeModel::ZoneTreeModel(QObject* parent)
    : QAbstractItemModel(parent) {}

void ZoneTreeModel::set_document(ZoneDocument* doc, const ClientContext* ctx) {
    doc_ = doc;
    ctx_ = ctx;
    rebuild();
}

void ZoneTreeModel::rebuild() {
    beginResetModel();
    build_tree();
    endResetModel();
}

ZoneTreeNode* ZoneTreeModel::node_at(const QModelIndex& index) const {
    if (!index.isValid()) return root_.get();
    return static_cast<ZoneTreeNode*>(index.internalPointer());
}

QModelIndex ZoneTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!root_) return {};
    auto* parent_node = parent.isValid()
        ? static_cast<ZoneTreeNode*>(parent.internalPointer())
        : root_.get();
    if (row < 0 || row >= static_cast<int>(parent_node->children.size())) return {};
    return createIndex(row, column, parent_node->children[row].get());
}

QModelIndex ZoneTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return {};
    auto* node = static_cast<ZoneTreeNode*>(child.internalPointer());
    if (!node || !node->parent || node->parent == root_.get()) return {};
    return createIndex(node->parent->row_in_parent, 0, node->parent);
}

int ZoneTreeModel::rowCount(const QModelIndex& parent) const {
    if (!root_) return 0;
    auto* node = parent.isValid()
        ? static_cast<ZoneTreeNode*>(parent.internalPointer())
        : root_.get();
    return static_cast<int>(node->children.size());
}

int ZoneTreeModel::columnCount(const QModelIndex&) const { return 2; }

QVariant ZoneTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return {};
    auto* node = static_cast<ZoneTreeNode*>(index.internalPointer());
    if (index.column() == 0) return QString::fromStdString(node->display_name);
    if (index.column() == 1) return QString::fromStdString(node->summary);
    return {};
}

QVariant ZoneTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    return section == 0 ? "Name" : "Value";
}

void ZoneTreeModel::build_tree() {
    root_.reset();
    if (!doc_ || !doc_->is_loaded()) return;

    auto& luz = doc_->luz();
    root_ = make_node(NodeType::Root, luz.zone_name.empty() ? "Zone" : luz.zone_name,
                       "v" + std::to_string(luz.version) + " world=" + std::to_string(luz.world_id));

    // Zone properties group
    auto props = make_node(NodeType::ZoneProperties, "Properties");
    add_child(root_.get(), std::move(props));

    // Scenes
    auto scenes = make_node(NodeType::ScenesGroup, "Scenes",
                            std::to_string(luz.scenes.size()));
    add_scene_nodes(scenes.get());
    add_child(root_.get(), std::move(scenes));

    // Boundaries
    auto bounds = make_node(NodeType::BoundariesGroup, "Boundaries",
                            std::to_string(luz.boundaries.size()));
    for (int i = 0; i < static_cast<int>(luz.boundaries.size()); ++i) {
        auto& b = luz.boundaries[i];
        auto bn = make_node(NodeType::Boundary,
                            "Boundary " + std::to_string(i),
                            "dest_map=" + std::to_string(b.dest_map_id), i);
        add_child(bounds.get(), std::move(bn));
    }
    add_child(root_.get(), std::move(bounds));

    // Transitions
    auto trans = make_node(NodeType::TransitionsGroup, "Transitions",
                           std::to_string(luz.transitions.size()));
    for (int i = 0; i < static_cast<int>(luz.transitions.size()); ++i) {
        auto& t = luz.transitions[i];
        std::string label = t.name.empty() ? "Transition " + std::to_string(i) : t.name;
        auto tn = make_node(NodeType::Transition, label,
                            std::to_string(t.points.size()) + " points", i);
        add_child(trans.get(), std::move(tn));
    }
    add_child(root_.get(), std::move(trans));

    // Paths
    auto paths = make_node(NodeType::PathsGroup, "Paths",
                           std::to_string(luz.paths.size()));
    add_path_nodes(paths.get());
    add_child(root_.get(), std::move(paths));
}

void ZoneTreeModel::add_scene_nodes(ZoneTreeNode* scenes_group) {
    auto& luz = doc_->luz();

    // Group scenes by scene ID
    std::map<uint32_t, std::vector<uint32_t>> scene_groups;
    for (uint32_t i = 0; i < static_cast<uint32_t>(luz.scenes.size()); ++i) {
        scene_groups[luz.scenes[i].id].push_back(i);
    }

    for (auto& [scene_id, indices] : scene_groups) {
        ZoneTreeNode* group_parent = scenes_group;

        // If multiple scenes share an ID, group them under a parent node
        if (indices.size() > 1) {
            std::string group_name = "Scene " + std::to_string(scene_id);
            if (!luz.scenes[indices[0]].name.empty())
                group_name = luz.scenes[indices[0]].name;
            auto gn = make_node(NodeType::ScenesGroup, group_name,
                                std::to_string(indices.size()) + " layers");
            group_parent = gn.get();
            add_child(scenes_group, std::move(gn));
        }

        for (uint32_t i : indices) {
            auto& sc = luz.scenes[i];
            std::string label = sc.name.empty() ? sc.filename : sc.name;
            auto sn = make_node(NodeType::Scene, label,
                                "id=" + std::to_string(sc.id) + " " + sc.filename, i);
            sn->scene_index = i;

            if (doc_->has_scene(i)) {
                auto& lvl = doc_->scene(i);

                if (lvl.has_environment) {
                    auto env = make_node(NodeType::EnvironmentGroup, "Environment");
                    env->scene_index = i;
                    add_child(sn.get(), std::move(env));
                }

                auto objs = make_node(NodeType::ObjectsGroup, "Objects",
                                      std::to_string(lvl.objects.size()));
                objs->scene_index = i;
                for (int j = 0; j < static_cast<int>(lvl.objects.size()); ++j) {
                    auto& obj = lvl.objects[j];
                    std::string obj_label = "LOT " + std::to_string(obj.lot);
                    if (ctx_) {
                        std::string name = ctx_->lot_name(obj.lot);
                        if (!name.empty()) obj_label = name + " (" + std::to_string(obj.lot) + ")";
                    }
                    std::string obj_summary = fmt_vec3(obj.position) + ldf_search_str(obj.config);
                    auto on = make_node(NodeType::Object, obj_label, obj_summary, j);
                    on->scene_index = i;
                    add_child(objs.get(), std::move(on));
                }
                add_child(sn.get(), std::move(objs));

                if (!lvl.particles.empty()) {
                    auto parts = make_node(NodeType::ParticlesGroup, "Particles",
                                           std::to_string(lvl.particles.size()));
                    parts->scene_index = i;
                    for (int j = 0; j < static_cast<int>(lvl.particles.size()); ++j) {
                        auto& p = lvl.particles[j];
                        auto pn = make_node(NodeType::Particle,
                                            p.effect_names.empty() ? "Particle " + std::to_string(j)
                                                                   : p.effect_names,
                                            fmt_vec3(p.position) + ldf_search_str(p.config), j);
                        pn->scene_index = i;
                        add_child(parts.get(), std::move(pn));
                    }
                    add_child(sn.get(), std::move(parts));
                }
            }

            add_child(group_parent, std::move(sn));
        }
    }
}

void ZoneTreeModel::add_path_nodes(ZoneTreeNode* paths_group) {
    auto& luz = doc_->luz();

    for (int i = 0; i < static_cast<int>(luz.paths.size()); ++i) {
        auto& path = luz.paths[i];
        const char* type_str = ClientContext::path_type_name(static_cast<uint32_t>(path.path_type));
        const char* behavior_str = ClientContext::path_behavior_name(static_cast<uint32_t>(path.behavior));

        auto pn = make_node(NodeType::Path, path.name,
                            std::string(type_str) + ", " + behavior_str, i);

        // Waypoints
        auto wps = make_node(NodeType::WaypointsGroup, "Waypoints",
                             std::to_string(path.waypoints.size()));
        wps->path_index = i;
        for (int j = 0; j < static_cast<int>(path.waypoints.size()); ++j) {
            auto wn = make_node(NodeType::Waypoint,
                                "Waypoint " + std::to_string(j),
                                fmt_vec3(path.waypoints[j].position), j);
            wn->path_index = i;
            add_child(wps.get(), std::move(wn));
        }
        add_child(pn.get(), std::move(wps));

        add_child(paths_group, std::move(pn));
    }
}

} // namespace zone_editor
