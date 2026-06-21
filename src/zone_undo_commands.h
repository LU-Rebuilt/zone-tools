#pragma once

#include "undo_commands.h"

#include "netdevil/zone/luz/luz_types.h"
#include "netdevil/zone/lvl/lvl_types.h"
#include "netdevil/common/ldf/ldf_types.h"

namespace zone_editor {

using lu_widgets::RefreshCallback;
using lu_widgets::FieldAccessor;
using lu_widgets::DirtyMarker;
using lu_widgets::EditFieldCommand;
using lu_widgets::AddItemCommand;
using lu_widgets::RemoveItemCommand;

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
