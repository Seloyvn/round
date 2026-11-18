import unreal


character = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Cleric/ClericDetails")
progression = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/Characters/Cleric/DA_ClericProgression")
if not character or not progression: raise RuntimeError("Cleric assets missing")
tiers = list(progression.get_editor_property("tiers"))
if len(tiers) != 4 or any(len(list(t.choices)) != 3 for t in tiers): raise RuntimeError("Character tree is not 4x3")
abilities = list(character.get_editor_property("abilities"))
if len(abilities) != 5: raise RuntimeError(f"Expected 5 abilities, found {len(abilities)}")
for ability in abilities:
    skill = ability.get_editor_property("skill_progression")
    levels = list(skill.get_editor_property("levels")) if skill else []
    if len(levels) != 12 or any(not str(level.title) for level in levels): raise RuntimeError(f"Invalid progression for {ability.get_name()}")
if abilities[2].get_editor_property("skill_progression").get_name() != "DA_SkillProgression_Rescue": raise RuntimeError("Rescue still references the wrong progression")
unreal.log("[validate_cleric_progressions] OK: 4 tiers, 12 character choices, 5 abilities, 60 ability choices, Rescue link fixed")
