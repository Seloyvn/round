import unreal


character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Gambler/GamblerDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Gambler/DA_GamblerProgression")
if not character or not progression: raise RuntimeError("Gambler assets missing")
tiers = list(progression.get_editor_property("tiers"))
if len(tiers) != 4 or any(len(list(t.choices)) != 3 for t in tiers): raise RuntimeError("Character tree is not 4x3")
abilities = list(character.get_editor_property("abilities"))
if len(abilities) != 5: raise RuntimeError(f"Expected 5 abilities, found {len(abilities)}")
for ability in abilities:
    skill = ability.get_editor_property("skill_progression")
    levels = list(skill.get_editor_property("levels")) if skill else []
    if len(levels) != 12 or any(not str(level.title) for level in levels): raise RuntimeError(f"Invalid progression for {ability.get_name()}")
unreal.log("[validate_gambler_progressions] OK: 4 tiers, 12 character choices, 5 abilities, 60 ability choices")
