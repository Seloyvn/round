import unreal


ABILITIES = {
    "Snipe": 11.0,
    "Concentrate": 12.0,
    "Aim": 13.0,
    "PiercingShot": 14.0,
    "SuppressingFire": 15.0,
}

for name, operation in ABILITIES.items():
    ability_path = f"/Game/TopDown/Data/Abilities/DA_Ability_{name}"
    progression_path = f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{name}"
    ability = unreal.EditorAssetLibrary.load_asset(ability_path)
    progression = unreal.EditorAssetLibrary.load_asset(progression_path)
    if not ability or not progression:
        raise RuntimeError(f"Character 2 asset missing: {name}")
    values = dict(ability.get_editor_property("values"))
    if values.get("CoreOperation") != operation:
        raise RuntimeError(f"{name} has CoreOperation {values.get('CoreOperation')}, expected {operation}")
    levels = list(progression.get_editor_property("levels"))
    if len(levels) != 12 or any(not str(level.title) for level in levels):
        raise RuntimeError(f"{name} progression is not a complete 4x3 tree")
    linked = ability.get_editor_property("skill_progression")
    if not linked or linked.get_path_name() != progression.get_path_name():
        raise RuntimeError(f"{name} does not reference its progression asset")

unreal.log("[validate_character2_core_abilities] OK: 5 Core abilities and 60 skillpoints")
