import unreal


registry = None
for candidate_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    candidate = unreal.EditorAssetLibrary.load_asset(candidate_path)
    if candidate and candidate.get_class().get_name() == "RoundAbilityRegistryData":
        registry = candidate
        break
if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")
registry.populate_engineer_ability_progression_defaults()

ABILITIES = {
    "Turret": (31.0, 0.0),
    "Mine": (32.0, 0.0),
    "ShieldEmitter": (33.0, 0.0),
    "SpotterBeacon": (34.0, 0.0),
    "Overload": (35.0, 0.0),
}

for name, (operation, relation) in ABILITIES.items():
    asset_path = f"/Game/TopDown/Data/Abilities/DA_Ability_{name}"
    ability = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not ability:
        raise RuntimeError(f"Could not load {asset_path}")
    values = dict(ability.get_editor_property("values"))
    values["CoreOperation"] = operation
    values["CoreTargetRelation"] = relation
    ability.set_editor_property("values", values)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset_path}")

if not unreal.EditorAssetLibrary.save_loaded_asset(registry, only_if_is_dirty=False):
    raise RuntimeError("Could not save Engineer ability registry")

for name in ABILITIES:
    progression_path = f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{name}"
    progression = unreal.EditorAssetLibrary.load_asset(progression_path)
    if not progression or not unreal.EditorAssetLibrary.save_loaded_asset(progression, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {progression_path}")

unreal.log("[migrate_engineer_core_abilities] Engineer abilities enabled for RoundCore")
