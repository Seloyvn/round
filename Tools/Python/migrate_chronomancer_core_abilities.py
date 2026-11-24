import unreal


ABILITIES = {
    "Accelerate": (36.0, 1.0),
    "TimeSiphon": (37.0, 2.0),
    "BorrowedTime": (38.0, 1.0),
    "Stasis": (39.0, 1.0),
    "TemporalAnchor": (40.0, 1.0),
}

registry = None
for path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    candidate = unreal.EditorAssetLibrary.load_asset(path)
    if candidate and candidate.get_class().get_name() == "RoundAbilityRegistryData":
        registry = candidate
        break
if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")
registry.populate_chronomancer_ability_progression_defaults()

for name, (operation, relation) in ABILITIES.items():
    path = f"/Game/TopDown/Data/Abilities/DA_Ability_{name}"
    ability = unreal.EditorAssetLibrary.load_asset(path)
    if not ability:
        raise RuntimeError(f"Could not load {path}")
    values = dict(ability.get_editor_property("values"))
    values["CoreOperation"] = operation
    values["CoreTargetRelation"] = relation
    ability.set_editor_property("values", values)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {path}")

if not unreal.EditorAssetLibrary.save_loaded_asset(registry, only_if_is_dirty=False):
    raise RuntimeError("Could not save ability registry")

unreal.log("[migrate_chronomancer_core_abilities] All Chronomancer abilities enabled for RoundCore")
