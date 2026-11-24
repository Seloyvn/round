import unreal


ABILITIES = {
    "ThornPatch": (41.0, 3.0),
    "Grove": (42.0, 3.0),
    "Rootsnare": (43.0, 2.0),
    "WildBloom": (44.0, 0.0),
    "BearForm": (45.0, 0.0),
}

registry = None
for path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    candidate = unreal.EditorAssetLibrary.load_asset(path)
    if candidate and candidate.get_class().get_name() == "RoundAbilityRegistryData":
        registry = candidate
        break
if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")
registry.populate_druid_ability_progression_defaults()

for name, (operation, relation) in ABILITIES.items():
    path = f"/Game/TopDown/Data/Abilities/DA_Ability_{name}"
    ability = unreal.EditorAssetLibrary.load_asset(path)
    if not ability:
        raise RuntimeError(f"Could not load {path}")
    values = dict(ability.get_editor_property("values"))
    values["CoreOperation"] = operation
    values["CoreTargetRelation"] = relation
    ability.set_editor_property("values", values)
    unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False)

unreal.EditorAssetLibrary.save_loaded_asset(registry, only_if_is_dirty=False)
unreal.log("[migrate_druid_core_abilities] Druid abilities enabled for RoundCore")
