import unreal


ABILITY_VALUES = {
    "/Game/TopDown/Data/Abilities/DA_Ability_PiercingShot": {},
    "/Game/TopDown/Data/Abilities/DA_Ability_Snipe": {
        "InitativeScaling": 4.0,
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Concentrate": {
        "DamagePerInitative": 0.01,
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Aim": {
        "DamageRange": 1.0,
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_SuppressingFire": {
        "MoveCostAddOnTarget": 2.0,
    },
}


for asset_path, values in ABILITY_VALUES.items():
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"Missing Character 2 ability asset: {asset_path}")

    asset.set_editor_property("values", values)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save Character 2 ability asset: {asset_path}")
    unreal.log(f"[fix_character2_ability_values] saved {asset_path}: {values}")

unreal.log("[fix_character2_ability_values] done")
