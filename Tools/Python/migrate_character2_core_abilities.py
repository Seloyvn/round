import unreal


registry = None
for candidate_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    candidate = unreal.EditorAssetLibrary.load_asset(candidate_path)
    if candidate and candidate.get_class().get_name() == "RoundAbilityRegistryData":
        registry = candidate
        break
if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")
registry.populate_character2_ability_progression_defaults()


ABILITIES = {
    "/Game/TopDown/Data/Abilities/DA_Ability_Snipe": (11.0, 2.0, {}),
    "/Game/TopDown/Data/Abilities/DA_Ability_Concentrate": (12.0, 0.0, {
        "ConcentrateDurationInitiative": 40.0,
    }),
    "/Game/TopDown/Data/Abilities/DA_Ability_Aim": (13.0, 2.0, {
        "AimFieldDurationInitiative": 160.0,
        "TrackingDurationInitiative": 160.0,
        "TrackingDamageMultiplier": 0.5,
        "CoverFireDamageMultiplier": 0.5,
    }),
    "/Game/TopDown/Data/Abilities/DA_Ability_PiercingShot": (14.0, 2.0, {
        "PreparationDurationInitiative": 160.0,
    }),
    "/Game/TopDown/Data/Abilities/DA_Ability_SuppressingFire": (15.0, 2.0, {
        "SuppressionDurationInitiative": 40.0,
    }),
}

for asset_path, (operation, relation, defaults) in ABILITIES.items():
    ability = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not ability:
        raise RuntimeError(f"Could not load {asset_path}")
    values = dict(ability.get_editor_property("values"))
    values["CoreOperation"] = operation
    values["CoreTargetRelation"] = relation
    for key, value in defaults.items():
        values.setdefault(key, value)
    ability.set_editor_property("values", values)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset_path}")

if not unreal.EditorAssetLibrary.save_loaded_asset(registry, only_if_is_dirty=False):
    raise RuntimeError("Could not save Character 2 ability registry")

for progression_name in ("Snipe", "Concentrate", "Aim", "PiercingShot", "SuppressingFire"):
    progression_path = f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{progression_name}"
    progression = unreal.EditorAssetLibrary.load_asset(progression_path)
    if not progression or not unreal.EditorAssetLibrary.save_loaded_asset(progression, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {progression_path}")

unreal.log("[migrate_character2_core_abilities] Character 2 abilities enabled for RoundCore")
