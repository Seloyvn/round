import unreal

ability_registry = None
character_registry = None
for path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        continue
    if asset.get_class().get_name() == "RoundAbilityRegistryData":
        ability_registry = asset
    elif asset.get_class().get_name() == "RoundCharacterRegistryData":
        character_registry = asset
if not ability_registry or not character_registry:
    raise RuntimeError("Registries not found")

character_registry.populate_paladin_progression_defaults()
ability_registry.populate_paladin_ability_progression_defaults()

character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Paladin/PaladinDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Paladin/DA_PaladinProgression")
abilities = list(character.get_editor_property("abilities"))
progressions = [a.get_editor_property("skill_progression") for a in abilities if a]
for asset in (character_registry, ability_registry, character, progression, *abilities, *progressions):
    if asset and not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")
unreal.log("[apply_paladin_progressions] Paladin progressions populated")
