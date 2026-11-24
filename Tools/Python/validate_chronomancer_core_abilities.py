import unreal


ABILITIES = {"Accelerate": 36.0, "TimeSiphon": 37.0}
for name, operation in ABILITIES.items():
    ability = unreal.EditorAssetLibrary.load_asset(f"/Game/TopDown/Data/Abilities/DA_Ability_{name}")
    progression = unreal.EditorAssetLibrary.load_asset(f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{name}")
    if not ability or not progression:
        raise RuntimeError(f"Chronomancer asset missing: {name}")
    values = dict(ability.get_editor_property("values"))
    if values.get("CoreOperation") != operation:
        raise RuntimeError(f"{name} has CoreOperation {values.get('CoreOperation')}, expected {operation}")
    levels = list(progression.get_editor_property("levels"))
    if len(levels) != 12:
        raise RuntimeError(f"{name} progression does not contain 12 skillpoints")
    linked = ability.get_editor_property("skill_progression")
    if not linked or linked.get_path_name() != progression.get_path_name():
        raise RuntimeError(f"{name} does not reference its progression asset")

unreal.log("[validate_chronomancer_core_abilities] OK: 2 Core abilities and 24 skillpoints")
