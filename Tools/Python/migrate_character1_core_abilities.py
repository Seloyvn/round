import unreal


ABILITIES = {
    "/Game/TopDown/Data/Abilities/DA_Ability_Knockback": 6.0,
    "/Game/TopDown/Data/Abilities/DA_Ability_Delay": 7.0,
    "/Game/TopDown/Data/Abilities/DA_Ability_Shackle": 8.0,
    "/Game/TopDown/Data/Abilities/DA_Ability_Bind": 9.0,
    "/Game/TopDown/Data/Abilities/DA_Ability_Taunt": 10.0,
}

for asset_path, operation in ABILITIES.items():
    ability = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not ability:
        raise RuntimeError(f"Could not load {asset_path}")
    values = dict(ability.get_editor_property("values"))
    values["CoreOperation"] = operation
    values["CoreTargetRelation"] = 2.0
    ability.set_editor_property("values", values)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset_path}")

unreal.log("[migrate_character1_core_abilities] Character 1 abilities enabled for RoundCore")
