#pragma once

#include "zone_document.h"
#include "client_context.h"

#include <QAbstractItemModel>
#include <memory>
#include <string>
#include <vector>

namespace zone_editor {

enum class NodeType {
    Root,
    ZoneProperties,
    ScenesGroup,
    Scene,
    EnvironmentGroup,
    ObjectsGroup,
    Object,
    ParticlesGroup,
    Particle,
    BoundariesGroup,
    Boundary,
    TransitionsGroup,
    Transition,
    PathsGroup,
    Path,
    WaypointsGroup,
    Waypoint,
};

struct ZoneTreeNode {
    NodeType type;
    int index = -1;
    int scene_index = -1;
    int path_index = -1;
    std::string display_name;
    std::string summary;

    std::vector<std::unique_ptr<ZoneTreeNode>> children;
    ZoneTreeNode* parent = nullptr;
    int row_in_parent = 0;
};

class ZoneTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit ZoneTreeModel(QObject* parent = nullptr);

    void set_document(ZoneDocument* doc, const ClientContext* ctx = nullptr);
    void rebuild();
    ZoneTreeNode* node_at(const QModelIndex& index) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    void build_tree();
    void add_scene_nodes(ZoneTreeNode* scenes_group);
    void add_path_nodes(ZoneTreeNode* paths_group);

    ZoneDocument* doc_ = nullptr;
    const ClientContext* ctx_ = nullptr;
    std::unique_ptr<ZoneTreeNode> root_;
};

} // namespace zone_editor
