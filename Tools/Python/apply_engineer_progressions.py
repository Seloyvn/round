import unreal


ability_registry = None
character_registry = None
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        continue
    class_name = asset.get_class().get_name()
    if class_name == "RoundAbilityRegistryData":
        ability_registry = asset
    elif class_name == "RoundCharacterRegistryData":
        character_registry = asset

if not ability_registry or not character_registry:
    raise RuntimeError("Character or ability registry was not found")

character_registry.populate_engineer_progression_defaults()
ability_registry.populate_engineer_ability_progression_defaults()

character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Engineer/EngineerDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Engineer/DA_EngineerProgression")
abilities = list(character.get_editor_property("abilities")) if character else []
progressions = [ability.get_editor_property("skill_progression") for ability in abilities if ability]

for asset in (character_registry, ability_registry, character, progression, *abilities, *progressions):
    if asset and not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")

unreal.log("[apply_engineer_progressions] Engineer character and ability progressions populated")
