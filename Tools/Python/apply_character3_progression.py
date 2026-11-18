import unreal


registry = None
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset and asset.get_class().get_name() == "RoundCharacterRegistryData":
        registry = asset
        break

if not registry:
    raise RuntimeError("No RoundCharacterRegistryData asset found")

registry.populate_character3_progression_defaults()

progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Character_3/DA_Character3Progression")
character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Character_3/Character3Details")
for asset in (registry, progression, character):
    if asset and not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")

unreal.log("[apply_character3_progression] Character3 progression populated")
