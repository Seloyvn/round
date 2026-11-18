import unreal


CHARACTER_PATH = "/Game/TopDown/Characters/Character_3/Character3Details"
DEATH_MARK_PATH = "/Game/TopDown/Data/Abilities/DA_Ability_DeathMark"


registry = None
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TopDown", recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset and asset.get_class().get_name() == "RoundAbilityRegistryData":
        registry = asset
        break

if not registry:
    raise RuntimeError("No RoundAbilityRegistryData asset found")

registry.populate_character3_ability_progression_defaults()

character = unreal.EditorAssetLibrary.load_asset(CHARACTER_PATH)
death_mark = unreal.EditorAssetLibrary.load_asset(DEATH_MARK_PATH)
if not character or not death_mark:
    raise RuntimeError(f"Missing Character3 or DeathMark asset: character={character}, death_mark={death_mark}")

base_stats = character.get_editor_property("base_stats")
base_stats.set_editor_property("attack_range", 1)
character.set_editor_property("base_stats", base_stats)

details = dict(character.get_editor_property("details"))
details["Range"] = 1.0
character.set_editor_property("details", details)

abilities = []
for ability in character.get_editor_property("abilities"):
    if ability and str(ability.get_editor_property("ability_id")).lower() == "smokestep":
        continue
    abilities.append(ability)
if not any(ability == death_mark for ability in abilities):
    abilities.append(death_mark)
character.set_editor_property("abilities", abilities)

character3_abilities = list(character.get_editor_property("abilities"))
character3_progressions = [
    ability.get_editor_property("skill_progression")
    for ability in character3_abilities
    if ability
]

for asset in (registry, *character3_abilities, *character3_progressions, character):
    if asset and not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {asset.get_path_name()}")

unreal.log("[apply_character3_death_mark] Character3 range set to 1; SmokeStep replaced with DeathMark")
