#include "Util.h"

namespace Util {

    std::string LightTypeToString(LightType type) {
        switch (type) {
            case LightType::LAMP_POST:      return "LAMP_POST";
            case LightType::HANGING_LIGHT:  return "HANGING_LIGHT";
            default: return "UNKNOWN";
        }
    }

    LightType StringToLightType(const std::string& str) {
        if (str == "LAMP_POST")         return LightType::LAMP_POST;
        if (str == "HANGING_LIGHT")     return LightType::HANGING_LIGHT;
        return LightType::LAMP_POST;
    }

    std::string PickUpTypeToString(PickUpType type) {
        switch (type) {
            case PickUpType::UNDEFINED:             return "UNDEFINED";
            case PickUpType::SHOTGUN_AMMO_BUCKSHOT: return "SHOTGUN_AMMO_BUCKSHOT";
            case PickUpType::SHOTGUN_AMMO_SLUG:     return "SHOTGUN_AMMO_SLUG";
            case PickUpType::AKS74U:                return "AKS74U";
            case PickUpType::REMINGTON_870:         return "REMINGTON_870";
        default: return "UNKNOWN";
        }
    }

    PickUpType StringToPickUpType(const std::string& str) {
        if (str == "SHOTGUN_AMMO_BUCKSHOT") return PickUpType::SHOTGUN_AMMO_BUCKSHOT;
        if (str == "SHOTGUN_AMMO_SLUG")     return PickUpType::SHOTGUN_AMMO_SLUG;
        if (str == "AKS74U")                return PickUpType::AKS74U;
        if (str == "REMINGTON_870")         return PickUpType::REMINGTON_870;        
        return PickUpType::UNDEFINED;
    }

    TrimType StringToTrimType(const std::string& str) {
        if (str == "PLASTER") return TrimType::PLASTER;
        if (str == "TIMBER") return TrimType::TIMBER;
        return TrimType::NONE;
    }

    std::string BlendingModeToString(BlendingMode mode) {
        switch (mode) {
            case BlendingMode::NONE:                return "NONE";
            case BlendingMode::BLENDED:             return "BLENDED";
            case BlendingMode::ALPHA_DISCARDED:     return "ALPHA_DISCARDED";
            case BlendingMode::HAIR_UNDER_LAYER:    return "HAIR_UNDER_LAYER";
            case BlendingMode::HAIR_TOP_LAYER:      return "HAIR_TOP_LAYER";
            case BlendingMode::DO_NOT_RENDER:       return "DO_NOT_RENDER";
            default: return "UNKNOWN";
        }
    }

    std::string ObjectTypeToString(ObjectType type) {
        switch (type) {
            case ObjectType::NONE:          return "NONE";
            case ObjectType::DECAL:         return "DECAL";
            case ObjectType::DOOR:          return "DOOR";
            case ObjectType::DOOR_FRAME:    return "DOOR_FRAME";
            case ObjectType::GAME_OBJECT:   return "GAME_OBJECT";
            case ObjectType::HEIGHT_MAP:    return "HEIGHT_MAP";
            case ObjectType::PLANE:   return "HOUSE_PLANE";
            case ObjectType::LIGHT:         return "LIGHT";
            case ObjectType::PICK_UP:       return "PICK_UP";
            case ObjectType::PIANO:         return "PIANO";
            case ObjectType::PIANO_KEY:     return "PIANO_KEY";
            case ObjectType::TREE:          return "TREE";
            case ObjectType::UNDEFINED:     return "UNDEFINED";
            case ObjectType::WALL:          return "WALL";
            case ObjectType::WALL_SEGMENT:  return "WALL_SEGMENT";
            case ObjectType::WINDOW:        return "WINDOW";
            default:                        return "UNKNOWN";
        }
    }

    std::string PhysicsTypeToString(PhysicsType type) {
        switch (type) {
            case PhysicsType::NONE:             return "NONE";
            case PhysicsType::RIGID_DYNAMIC:    return "RIGID_DYNAMIC";
            case PhysicsType::RIGID_STATIC:     return "RIGID_STATIC";
            case PhysicsType::HEIGHT_FIELD:     return "HEIGHT_FIELD";
            case PhysicsType::GROUND_PLANE:     return "GROUND_PLANE";
            case PhysicsType::UNDEFINED:        return "UNDEFINED";
            default:                            return "UNKNOWN";
        }
    }

    std::string TrimTypeToString(TrimType type) {
        switch (type) {
            case TrimType::NONE:             return "NONE";
            case TrimType::PLASTER :         return "PLASTER";
            case TrimType::TIMBER:           return "TIMBER";
            default:                         return "UNKNOWN";
        }
    }

    BlendingMode StringToBlendingMode(const std::string& str) {
        if (str == "BLENDED") return BlendingMode::BLENDED;
        if (str == "ALPHA_DISCARDED") return BlendingMode::ALPHA_DISCARDED;
        if (str == "HAIR_UNDER_LAYER") return BlendingMode::HAIR_UNDER_LAYER;
        if (str == "HAIR_TOP_LAYER") return BlendingMode::HAIR_TOP_LAYER;
        if (str == "DO_NOT_RENDER") return BlendingMode::DO_NOT_RENDER;
        return BlendingMode::NONE;
    }

    std::string WeaponActionToString(const WeaponAction& weaponAction) {
        switch (weaponAction) {
            case WeaponAction::IDLE:                          return "IDLE";
            case WeaponAction::FIRE:                          return "FIRE";
            case WeaponAction::RELOAD:                        return "RELOAD";
            case WeaponAction::RELOAD_FROM_EMPTY:             return "RELOAD_FROM_EMPTY";
            case WeaponAction::DRAW_BEGIN:                    return "DRAW_BEGIN";
            case WeaponAction::DRAWING:                       return "DRAWING";
            case WeaponAction::DRAWING_FIRST:                 return "DRAWING_FIRST";
            case WeaponAction::DRAWING_WITH_SHOTGUN_PUMP:     return "DRAWING_WITH_SHOTGUN_PUMP";
            case WeaponAction::SPAWNING:                      return "SPAWNING";
            case WeaponAction::SHOTGUN_RELOAD_BEGIN:          return "SHOTGUN_RELOAD_BEGIN";
            case WeaponAction::SHOTGUN_RELOAD_SINGLE_SHELL:   return "SHOTGUN_RELOAD_SINGLE_SHELL";
            case WeaponAction::SHOTGUN_RELOAD_DOUBLE_SHELL:   return "SHOTGUN_RELOAD_DOUBLE_SHELL";
            case WeaponAction::SHOTGUN_RELOAD_END:            return "SHOTGUN_RELOAD_END";
            case WeaponAction::SHOTGUN_RELOAD_END_WITH_PUMP:  return "SHOTGUN_RELOAD_END_WITH_PUMP";
            case WeaponAction::ADS_IN:                        return "ADS_IN";
            case WeaponAction::ADS_OUT:                       return "ADS_OUT";
            case WeaponAction::ADS_IDLE:                      return "ADS_IDLE";
            case WeaponAction::ADS_FIRE:                      return "ADS_FIRE";
            case WeaponAction::MELEE:                         return "MELEE";
            case WeaponAction::TOGGLING_AUTO:                 return "TOGGLING_AUTO";
            case WeaponAction::DRY_FIRE:                      return "DRY_FIRE";
            default:                                          return "UNDEFINED";
        }
    }

    std::string ImageDataTypeToString(const ImageDataType& imageDataType) {
        switch (imageDataType) {
            case ImageDataType::UNCOMPRESSED: return "UNCOMPRESSED";
            case ImageDataType::COMPRESSED:   return "COMPRESSED";
            case ImageDataType::EXR:          return "EXR";
            default:                          return "UNDEFINED";
        }
    }
    std::string EditorModeToString(const EditorMode& editorMode) {
        switch (editorMode) {
            case EditorMode::HOUSE_EDITOR:      return "HOUSE_EDITOR";
            case EditorMode::MAP_EDITOR:        return "MAP_EDITOR";
            case EditorMode::SECTOR_EDITOR:     return "SECTOR_EDITOR";
            case EditorMode::HEIGHTMAP_EDITOR:  return "HEIGHTMAP_EDITOR";
            case EditorMode::WEAPON_EDITOR:     return "WEAPON_EDITOR";
            default:                            return "UNDEFINED";
        }
    }

    std::string DebugRenderModeToString(const DebugRenderMode& mode) {
        switch (mode) {
            case NONE:                                                  return "NONE";
            case DECALS:                                                return "DECALS";
            case PATHFINDING_RECAST:                                    return "PATHFINDING_RECAST";
            case PHYSX_ALL:                                             return "PHYSX_ALL";
            case PHYSX_RAYCAST:                                         return "PHYSX_RAYCAST";
            case PHYSX_COLLISION:                                       return "PHYSX_COLLISION";
            case RAYTRACE_LAND:                                         return "RAYTRACE_LAND";
            case PHYSX_EDITOR:                                          return "PHYSX_EDITOR";
            case BOUNDING_BOXES:                                        return "BOUNDING_BOXES";
            case RTX_LAND_AABBS:                                        return "RTX_LAND_AABBS";
            case RTX_LAND_TRIS:                                         return "RTX_LAND_TRIS";
            case RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE:             return "RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE";
            case RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES:         return "RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES";
            case RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES: return "RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES";
            case CLIPPING_CUBES:                                        return "CLIPPING_CUBES";
            case HOUSE_GEOMETRY:                                        return "HOUSE_GEOMETRY";
            case BONES:                                                 return "BONES";
            case BONE_TANGENTS:                                         return "BONE_TANGENTS";
            case DEBUG_LINE_MODE_COUNT:                                 return "DEBUG_LINE_MODE_COUNT";
            default: return "UNDEFINED";
        }
    }
    
    std::string ViewportModeToString(const ShadingMode& viewportMode) {
        switch (viewportMode) {
            case ShadingMode::SHADED:            return "SHADED";
            case ShadingMode::WIREFRAME:         return "WIREFRAME";
            case ShadingMode::WIREFRAME_OVERLAY: return "WIREFRAME_OVERLAY";
            default:                              return "UNDEFINED";
        }
    }

    std::string CameraViewToString(const CameraView& cameraView) {
        switch (cameraView) {
            case CameraView::PERSPECTIVE:   return "PERSPECTIVE";
            case CameraView::ORTHO:         return "ORTHOGRAPHIC";
            case CameraView::FRONT:         return "FRONT";
            case CameraView::BACK:          return "BACK";
            case CameraView::TOP:           return "TOP";
            case CameraView::BOTTOM:        return "BOTTOM";
            case CameraView::LEFT:          return "LEFT";
            case CameraView::RIGHT:         return "RIGHT";
            default:                        return "UNDEFINED";
        }
    }

    std::string EditorStateToString(const EditorState& editorState) {
        switch (editorState) {
            case EditorState::IDLE:                             return "IDLE";
            case EditorState::RESIZING_HORIZONTAL:              return "RESIZING_HORIZONTAL";
            case EditorState::RESIZING_VERTICAL:                return "RESIZING_VERTICAL";
            case EditorState::RESIZING_HORIZONTAL_VERTICAL:     return "RESIZING_HORIZONTAL_VERTICAL";
            case EditorState::GIZMO_TRANSLATING:                return "GIZMO_TRANSLATING";
            case EditorState::GIZMO_SCALING:                    return "GIZMO_SCALING";
            case EditorState::GIZMO_ROTATING:                   return "GIZMO_ROTATING";
            case EditorState::DRAGGING_SELECT_RECT:             return "DRAGGING_SELECT_RECT";
            default:                                            return "UNDEFINED";
        }
    }

    ObjectType IntToEnum(int value) {
        return static_cast<ObjectType>(value);
    }

    int32_t EnumToInt(ObjectType type) {
        return static_cast<int32_t>(type);
    }
}
