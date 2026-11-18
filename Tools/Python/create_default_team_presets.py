import unreal


ASSET_PATH = "/Game/TopDown/Data/DA_DefaultTeamPresets"
REGISTRY_PATH = "/Game/TopDown/DA_RoundGameDataRegistry"

ABILITIES = {
    "Character1": ["Knockback", "Delay", "Shackle", "Bind"],
    "Character2": ["PiercingShot", "Snipe", "Concentrate", "Aim"],
    "Character3": ["Shadowstrike", "Armorpen", "Execute", "HookShot"],
    "Character4": ["Fireball", "Lightningstrike", "Rockblast", "Icebeam"],
    "Character5": ["Dash", "Decimate", "Heal", "Rend"],
    "Bladecaller": ["LaunchBlade", "DriveBlades", "BladeSwap", "BladeCall"],
    "Cleric": ["DelayHeal", "Pulse", "Rescue", "Shield"],
    "Engineer": ["Turret", "Mine", "ShieldEmitter", "SpotterBeacon"],
    "Chronomancer": ["Accelerate", "BorrowedTime", "Stasis", "TemporalAnchor"],
    "Druid": ["ThornPatch", "Grove", "Rootsnare", "WildBloom"],
    "Dragoon": ["Lunge", "Overshoot", "ExecuteDive", "HoldtheLine"],
    "Geomancer": ["StoneWall", "Faultline", "Detonate", "Tremor"],
    "Gambler": ["QuickDraw", "DoubleDraw", "LowStakes", "Flush"],
    "Paladin": ["Smite", "Condem", "SanctifyGround", "Bulwark"],
}

TEAMS = [
    ("Frontdruck", ["Character1", "Character5", "Paladin", "Cleric", "Dragoon"]),
    ("Fernkampf", ["Character2", "Character4", "Engineer", "Chronomancer", "Geomancer"]),
    ("Jagdkommando", ["Character3", "Bladecaller", "Gambler", "Character5", "Dragoon"]),
    ("Raumkontrolle", ["Druid", "Geomancer", "Engineer", "Cleric", "Character1"]),
]

POSITIONS = [(-2.0, 0.0), (-1.0, 1.0), (0.0, 0.0), (1.0, -1.0), (2.0, 0.0)]


def packed_choices(*choices):
    result = 0
    for tier_index, choice in enumerate(choices):
        result |= (choice & 0x3) << (tier_index * 2)
    return result


def make_character(character_id, position):
    character = unreal.RoundCharacterEntry()
    character.set_editor_property("character", character_id)
    character.set_editor_property("start_position", unreal.Vector2D(*position))
    character.set_editor_property("character_skill_selections", packed_choices(1, 1, 1, 1))
    character.set_editor_property("character_skillpoints", 4)

    skills = []
    for ability_id in ABILITIES[character_id]:
        skill = unreal.RoundSkillEntry()
        skill.set_editor_property("name", ability_id)
        skill.set_editor_property("points", packed_choices(1, 1, 1))
        skills.append(skill)

    character.set_editor_property("skills", skills)
    character.set_editor_property("skillpoints", 12)
    return character


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RoundTeamPresetData)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_DefaultTeamPresets", "/Game/TopDown/Data", unreal.RoundTeamPresetData, factory
    )

if not asset:
    raise RuntimeError("Could not create DA_DefaultTeamPresets")

team_definitions = []
for team_name, character_ids in TEAMS:
    team = unreal.RoundTeamDefinition()
    team.set_editor_property("name", team_name)
    team.set_editor_property("format", unreal.RoundFormat.FORMAT5X16)
    team.set_editor_property(
        "characters",
        [make_character(character_id, POSITIONS[index]) for index, character_id in enumerate(character_ids)],
    )
    team_definitions.append(team)

asset.set_editor_property("teams", team_definitions)
unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

registry = unreal.EditorAssetLibrary.load_asset(REGISTRY_PATH)
if not registry:
    raise RuntimeError(f"Missing registry: {REGISTRY_PATH}")

registry.set_editor_property("default_team_presets", asset)
unreal.EditorAssetLibrary.save_loaded_asset(registry, only_if_is_dirty=False)
unreal.log(f"[create_default_team_presets] Saved {len(team_definitions)} teams with five characters and 16 points each")
