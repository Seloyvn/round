import unreal


TARGETS = {
    "Character2": "/Game/TopDown/Characters/Character_2/DA_Character2Progression",
    "Character3": "/Game/TopDown/Characters/Character_3/DA_Character3Progression",
    "Character4": "/Game/TopDown/Characters/Character_4/DA_Character4Progression",
    "Character5": "/Game/TopDown/Characters/Character_5/DA_Character5Progression",
    "Bladecaller": "/Game/TopDown/Characters/Bladecaller/DA_BladecallerProgression",
    "Chronomancer": "/Game/TopDown/Characters/Chronomancer/DA_ChronomancerProgression",
    "Cleric": "/Game/TopDown/Characters/Cleric/DA_ClericProgression",
    "Dragoon": "/Game/TopDown/Characters/Dragoon/DA_DragoonProgression",
    "Druid": "/Game/TopDown/Characters/Druid/DA_DruidProgression",
    "Engineer": "/Game/TopDown/Characters/Engineer/DA_EngineerProgression",
    "Gambler": "/Game/TopDown/Characters/Gambler/DA_GamblerProgression",
    "Geomancer": "/Game/TopDown/Characters/Geomancer/DA_GeomancerProgression",
    "Paladin": "/Game/TopDown/Characters/Paladin/DA_PaladinProgression",
}


def main():
    for character_id, path in TARGETS.items():
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not asset:
            unreal.log_error(f"PROGRESSION_AUDIT missing {character_id} {path}")
            continue
        tiers = list(asset.get_editor_property("tiers"))
        counts = [len(list(tier.choices)) for tier in tiers]
        titles = [str(tier.title) for tier in tiers]
        unreal.log(
            f"PROGRESSION_AUDIT {character_id} tiers={len(tiers)} "
            f"choices={counts} titles={titles}"
        )


if __name__ == "__main__":
    main()
