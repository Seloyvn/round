import unreal


registry = None
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset and asset.get_class().get_name() == "RoundAbilityRegistryData":
        registry = asset
        break

if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")

registry.populate_character4_ability_progression_defaults()

character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Character_4/Character4Details")
abilities = list(character.get_editor_property("abilities")) if character else []
progressions = [
    ability.get_editor_property("skill_progression")
    for ability in abilities
    if ability
]

for asset in (registry, *abilities, *progressions, character):
    if asset and not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")

unreal.log("[apply_character4_ability_progressions] Character4 ability progressions populated")
