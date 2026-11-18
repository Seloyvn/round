import unreal


ASSET_PATH = "/Game/TopDown/Data/DA_DefaultTeamPresets"


def selected_tiers(packed_value):
    count = 0
    for tier_index in range(4):
        if ((packed_value >> (tier_index * 2)) & 0x3) == 0:
            break
        count += 1
    return count


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    raise RuntimeError(f"Missing asset: {ASSET_PATH}")

teams = asset.get_editor_property("teams")
if len(teams) < 2:
    raise RuntimeError("At least two default teams are required")

for team in teams:
    name = team.get_editor_property("name")
    characters = team.get_editor_property("characters")
    if len(characters) != 5:
        raise RuntimeError(f"Team {name} has {len(characters)} characters instead of 5")

    for character in characters:
        character_id = character.get_editor_property("character")
        spent = selected_tiers(character.get_editor_property("character_skill_selections"))
        spent += sum(selected_tiers(skill.get_editor_property("points")) for skill in character.get_editor_property("skills"))
        if spent != 16:
            raise RuntimeError(f"{name}/{character_id} has {spent} spent points instead of 16")

unreal.log(f"[validate_default_team_presets] OK: {len(teams)} teams, five characters and 16 points per character")
