import unreal


ABILITIES = {
    "Lunge": 47.0,
    "Overshoot": 48.0,
    "ExecuteDive": 49.0,
    "HoldtheLine": 50.0,
    "Repel": 51.0,
}

for name, operation in ABILITIES.items():
    ability = unreal.EditorAssetLibrary.load_asset(f"/Game/TopDown/Data/Abilities/DA_Ability_{name}")
    progression = unreal.EditorAssetLibrary.load_asset(
        f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{name}")
    if not ability or not progression:
        raise RuntimeError(f"Dragoon asset missing: {name}")
    if dict(ability.get_editor_property("values")).get("CoreOperation") != operation:
        raise RuntimeError(f"{name} has incorrect CoreOperation")
    if len(list(progression.get_editor_property("levels"))) != 12:
        raise RuntimeError(f"{name} progression does not contain 12 skillpoints")

unreal.log("[validate_dragoon_core_abilities] OK: 5 Core abilities and 60 skillpoints")
