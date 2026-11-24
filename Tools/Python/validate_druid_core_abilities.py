import unreal


ABILITIES = {"ThornPatch": 41.0, "Grove": 42.0, "Rootsnare": 43.0,
             "WildBloom": 44.0, "BearForm": 45.0}
for name, operation in ABILITIES.items():
    ability = unreal.EditorAssetLibrary.load_asset(f"/Game/TopDown/Data/Abilities/DA_Ability_{name}")
    progression = unreal.EditorAssetLibrary.load_asset(f"/Game/TopDown/Data/SkillProgression/DA_SkillProgression_{name}")
    if not ability or not progression:
        raise RuntimeError(f"Druid asset missing: {name}")
    if dict(ability.get_editor_property("values")).get("CoreOperation") != operation:
        raise RuntimeError(f"{name} has incorrect CoreOperation")
    if len(list(progression.get_editor_property("levels"))) != 12:
        raise RuntimeError(f"{name} progression does not contain 12 skillpoints")

unreal.log("[validate_druid_core_abilities] OK: 5 Core abilities and 60 skillpoints")
