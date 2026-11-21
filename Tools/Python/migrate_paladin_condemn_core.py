import unreal


ASSET_PATH = "/Game/TopDown/Data/Abilities/DA_Ability_Condem"

ability = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not ability:
    raise RuntimeError(f"Could not load {ASSET_PATH}")

values = dict(ability.get_editor_property("values"))
values["CoreOperation"] = 1.0
values["CoreTargetRelation"] = 2.0
ability.set_editor_property("values", values)

if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {ASSET_PATH}")

unreal.log("[migrate_paladin_condemn_core] Condemn enabled for RoundCore")
