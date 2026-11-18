import unreal


character_paths = [
    "/Game/TopDown/Characters/Character_1/Character1Details",
    "/Game/TopDown/Characters/Character_2/Character2Details",
    "/Game/TopDown/Characters/Character_3/Character3Details",
    "/Game/TopDown/Characters/Character_4/Character4Details",
    "/Game/TopDown/Characters/Character_5/Character5Details",
    "/Game/TopDown/Characters/Bladecaller/BladecallerDetails",
    "/Game/TopDown/Characters/Cleric/ClericDetails",
    "/Game/TopDown/Characters/Engineer/EngineerDetails",
    "/Game/TopDown/Characters/Chronomancer/ChronomancerDetails",
    "/Game/TopDown/Characters/Druid/DruidDetails",
    "/Game/TopDown/Characters/Dragoon/DragoonDetails",
    "/Game/TopDown/Characters/Geomancer/GeomancerDetails",
    "/Game/TopDown/Characters/Gambler/GamblerDetails",
    "/Game/TopDown/Characters/Paladin/PaladinDetails",
]

abilities = {}
for path in character_paths:
    character = unreal.EditorAssetLibrary.load_asset(path)
    if not character:
        raise RuntimeError(f"Missing character: {path}")
    for ability in character.get_editor_property("abilities"):
        if ability:
            abilities[ability.get_path_name()] = ability

empty = [path for path, ability in abilities.items() if not str(ability.get_editor_property("description")).strip()]
if empty:
    raise RuntimeError("Abilities without description: " + ", ".join(empty))
if len(abilities) != 70:
    raise RuntimeError(f"Expected 70 regular abilities, found {len(abilities)}")

unreal.log(f"[validate_functional_ability_descriptions] OK: {len(abilities)} abilities have functional descriptions")
