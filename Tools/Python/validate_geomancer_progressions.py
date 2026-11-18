import unreal


character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Geomancer/GeomancerDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Geomancer/DA_GeomancerProgression")
if not character or not progression:
    raise RuntimeError("Geomancer character or progression asset is missing")

tiers = list(progression.get_editor_property("tiers"))
if len(tiers) != 4 or any(len(list(tier.get_editor_property("choices"))) != 3 for tier in tiers):
    raise RuntimeError("Geomancer character progression is not 4x3")

abilities = list(character.get_editor_property("abilities"))
if len(abilities) != 5:
    raise RuntimeError(f"Expected 5 Geomancer abilities, found {len(abilities)}")

for ability in abilities:
    skill_progression = ability.get_editor_property("skill_progression")
    if not skill_progression:
        raise RuntimeError(f"{ability.get_name()} has no progression")
    levels = list(skill_progression.get_editor_property("levels"))
    if len(levels) != 12:
        raise RuntimeError(f"{ability.get_name()} has {len(levels)} levels instead of 12")

unreal.log("[validate_geomancer_progressions] OK: 4 tiers, 12 character choices, 5 abilities, 60 ability choices")
