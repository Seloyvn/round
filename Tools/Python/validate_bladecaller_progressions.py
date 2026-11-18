import unreal


character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Bladecaller/BladecallerDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Bladecaller/DA_BladecallerProgression")
if not character or not progression:
    raise RuntimeError("Bladecaller assets are missing")
tiers = list(progression.get_editor_property("tiers"))
if len(tiers) != 4 or any(len(list(tier.choices)) != 3 for tier in tiers):
    raise RuntimeError("Bladecaller character progression is not 4x3")
abilities = list(character.get_editor_property("abilities"))
if len(abilities) != 5:
    raise RuntimeError(f"Expected 5 abilities, found {len(abilities)}")
for ability in abilities:
    levels = list(ability.get_editor_property("skill_progression").get_editor_property("levels"))
    if len(levels) != 12 or any(not str(level.title) for level in levels):
        raise RuntimeError(f"Invalid progression for {ability.get_name()}")
unreal.log("[validate_bladecaller_progressions] OK: 4 tiers, 12 character choices, 5 abilities, 60 ability choices")
