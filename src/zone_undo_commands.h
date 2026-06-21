#pragma once

#include "zone_document.h"
#include "netdevil/zone/luz/luz_types.h"
#include "netdevil/zone/lvl/lvl_types.h"
#include "netdevil/common/ldf/ldf_types.h"

#include <QUndoCommand>
#include <functional>
#include <string>
#include <vector>

namespace zone_editor {

using RefreshCallback = std::function<void()>;

template<typename T>
using FieldAccessor = std::function<T&()>;

using DirtyMarker = std::function<void()>;

template<typename T>
class EditFieldCommand : public QUndoCommand {
public:
    EditFieldCommand(const QString& description,
                     FieldAccessor<T> accessor,
                     T old_value, T new_value,
                     DirtyMarker dirty, RefreshCallback refresh)
        : QUndoCommand(description)
        , accessor_(std::move(accessor))
        , old_(std::move(old_value))
        , new_(std::move(new_value))
        , dirty_(std::move(dirty))
        , refresh_(std::move(refresh)) {}

    void redo() override {
        if (first_redo_) { first_redo_ = false; return; }
        accessor_() = new_;
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

    void undo() override {
        accessor_() = old_;
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

    int id() const override { return merge_id_; }
    void set_merge_id(int mid) { merge_id_ = mid; }

    bool mergeWith(const QUndoCommand* other) override {
        auto* o = dynamic_cast<const EditFieldCommand<T>*>(other);
        if (!o || merge_id_ < 0) return false;
        if (o->merge_id_ != merge_id_) return false;
        new_ = o->new_;
        return true;
    }

private:
    FieldAccessor<T> accessor_;
    T old_, new_;
    DirtyMarker dirty_;
    RefreshCallback refresh_;
    bool first_redo_ = true;
    int merge_id_ = -1;
};

template<typename T>
class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(const QString& description,
                   std::function<std::vector<T>&()> vec_accessor,
                   T item, int insert_index,
                   DirtyMarker dirty, RefreshCallback refresh)
        : QUndoCommand(description)
        , vec_(std::move(vec_accessor))
        , item_(std::move(item))
        , index_(insert_index)
        , dirty_(std::move(dirty))
        , refresh_(std::move(refresh)) {}

    void redo() override {
        if (first_redo_) { first_redo_ = false; return; }
        auto& v = vec_();
        if (index_ < 0 || index_ >= static_cast<int>(v.size()))
            v.push_back(item_);
        else
            v.insert(v.begin() + index_, item_);
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

    void undo() override {
        auto& v = vec_();
        int idx = (index_ < 0 || index_ >= static_cast<int>(v.size()))
            ? static_cast<int>(v.size()) - 1 : index_;
        if (idx >= 0 && idx < static_cast<int>(v.size()))
            v.erase(v.begin() + idx);
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

private:
    std::function<std::vector<T>&()> vec_;
    T item_;
    int index_;
    DirtyMarker dirty_;
    RefreshCallback refresh_;
    bool first_redo_ = true;
};

template<typename T>
class RemoveItemCommand : public QUndoCommand {
public:
    RemoveItemCommand(const QString& description,
                      std::function<std::vector<T>&()> vec_accessor,
                      int index,
                      DirtyMarker dirty, RefreshCallback refresh)
        : QUndoCommand(description)
        , vec_(std::move(vec_accessor))
        , index_(index)
        , dirty_(std::move(dirty))
        , refresh_(std::move(refresh)) {
        snapshot_ = vec_()[index_];
    }

    void redo() override {
        if (first_redo_) { first_redo_ = false; return; }
        auto& v = vec_();
        v.erase(v.begin() + index_);
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

    void undo() override {
        auto& v = vec_();
        v.insert(v.begin() + index_, snapshot_);
        if (dirty_) dirty_();
        if (refresh_) refresh_();
    }

private:
    std::function<std::vector<T>&()> vec_;
    int index_;
    T snapshot_;
    DirtyMarker dirty_;
    RefreshCallback refresh_;
    bool first_redo_ = true;
};

using EditFloatCmd  = EditFieldCommand<float>;
using EditU32Cmd    = EditFieldCommand<uint32_t>;
using EditU64Cmd    = EditFieldCommand<uint64_t>;
using EditS32Cmd    = EditFieldCommand<int32_t>;
using EditU16Cmd    = EditFieldCommand<uint16_t>;
using EditU8Cmd     = EditFieldCommand<uint8_t>;
using EditBoolCmd   = EditFieldCommand<bool>;
using EditStringCmd = EditFieldCommand<std::string>;
using EditVec3Cmd   = EditFieldCommand<lu::assets::Vec3>;
using EditQuatCmd   = EditFieldCommand<lu::assets::Quat>;

using AddObjectCmd     = AddItemCommand<lu::assets::LvlObject>;
using RemoveObjectCmd  = RemoveItemCommand<lu::assets::LvlObject>;
using AddParticleCmd   = AddItemCommand<lu::assets::LvlParticle>;
using RemoveParticleCmd = RemoveItemCommand<lu::assets::LvlParticle>;
using AddPathCmd       = AddItemCommand<lu::assets::LuzPath>;
using RemovePathCmd    = RemoveItemCommand<lu::assets::LuzPath>;
using AddWaypointCmd   = AddItemCommand<lu::assets::LuzWaypoint>;
using RemoveWaypointCmd = RemoveItemCommand<lu::assets::LuzWaypoint>;
using AddBoundaryCmd   = AddItemCommand<lu::assets::LuzBoundary>;
using RemoveBoundaryCmd = RemoveItemCommand<lu::assets::LuzBoundary>;
using AddTransitionCmd = AddItemCommand<lu::assets::LuzTransition>;
using RemoveTransitionCmd = RemoveItemCommand<lu::assets::LuzTransition>;
using AddLdfEntryCmd   = AddItemCommand<lu::assets::LdfEntry>;
using RemoveLdfEntryCmd = RemoveItemCommand<lu::assets::LdfEntry>;
using AddLdfConfigEntryCmd   = AddItemCommand<std::pair<std::string, std::string>>;
using RemoveLdfConfigEntryCmd = RemoveItemCommand<std::pair<std::string, std::string>>;

} // namespace zone_editor
