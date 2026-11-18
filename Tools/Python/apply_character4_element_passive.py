import unreal


CHARACTER_PATH = "/Game/TopDown/Characters/Character_4/Character4Details"


character = unreal.EditorAssetLibrary.load_asset(CHARACTER_PATH)
if not character:
    raise RuntimeError("Missing Character4 data asset")

details = dict(character.get_editor_property("details"))
details.update({
    "ElementFireDamageMultiplier": 1.25,
    "ElementIceRangeAdd": 2.0,
    "ElementLightningCooldownMultiplier": 0.75,
    "ElementRockDefensePerEnemyHit": 10.0,
    "ElementRockMaxDefenseStacks": 4.0,
    "ElementRockDefenseDurationInitiative": 80.0,
})
character.set_editor_property("details", details)

character_registry = None
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset and asset.get_class().get_name() == "RoundCharacterRegistryData":
        character_registry = asset
        break

if not character_registry:
    raise RuntimeError("No RoundCharacterRegistryData asset found")

character_registry.populate_character4_progression_defaults()
progression = character.get_editor_property("character_progression")

if not unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False):
    raise RuntimeError("Could not save Character4 data asset")
if progression and not unreal.EditorAssetLibrary.save_loaded_asset(progression, only_if_is_dirty=False):
    raise RuntimeError("Could not save Character4 progression data asset")
if not unreal.EditorAssetLibrary.save_loaded_asset(character_registry, only_if_is_dirty=False):
    raise RuntimeError("Could not save character registry")

unreal.log("[apply_character4_element_passive] Saved elemental chain values and character progression")
