import unreal


CHARACTER_ASSETS = [
    "/Game/TopDown/Characters/Character_1/Character1Details",
    "/Game/TopDown/Characters/Character_3/Character3Details",
    "/Game/TopDown/Characters/Bladecaller/BladecallerDetails",
    "/Game/TopDown/Characters/Cleric/ClericDetails",
    "/Game/TopDown/Characters/Character_4/Character4Details",
    "/Game/TopDown/Characters/Character_5/Character5Details",
    "/Game/TopDown/Characters/Character_2/Character2Details",
    "/Game/TopDown/Characters/Engineer/EngineerDetails",
    "/Game/TopDown/Characters/Chronomancer/ChronomancerDetails",
    "/Game/TopDown/Characters/Druid/DruidDetails",
    "/Game/TopDown/Characters/Dragoon/DragoonDetails",
    "/Game/TopDown/Characters/Geomancer/GeomancerDetails",
    "/Game/TopDown/Characters/Gambler/GamblerDetails",
    "/Game/TopDown/Characters/Paladin/PaladinDetails",
]


def _log(text):
    unreal.log("[dump_new_character_values] " + str(text))


def _dump_character(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        _log(f"missing character asset: {asset_path}")
        return

    stats = asset.get_editor_property("base_stats")
    _log(f"character={asset_path}")
    _log(
        " stats="
        f"hp={stats.max_health} atk={stats.attack} def={stats.defense} init={stats.max_initiative} "
        f"vision={stats.vision_range} range={stats.attack_range} move={stats.move_cost} "
        f"atk_cost={stats.attack_cost} turn_delay={stats.turn_delay}"
    )
    _log(f" details={asset.get_editor_property('details')}")
    character_progression = asset.get_editor_property("character_progression")
    if character_progression:
        tiers = character_progression.get_editor_property("tiers")
        _log(f" character_progression={character_progression.get_path_name()} tier_count={len(tiers)}")
        for tier_index, tier in enumerate(tiers):
            choices = tier.choices
            _log(f"  tier[{tier_index}] title={tier.title} choice_count={len(choices)}")
            for choice_index, choice in enumerate(choices):
                _log(
                    f"   choice[{choice_index}] title={choice.title} "
                    f"description={choice.description} values={choice.values}"
                )

    abilities = asset.get_editor_property("abilities")
    for ability in abilities:
        if not ability:
            continue
        _log(f" ability_ref={ability.get_path_name()}")
        _dump_ability(ability)


def _dump_ability(asset_or_path):
    asset = asset_or_path
    if isinstance(asset_or_path, str):
        asset = unreal.EditorAssetLibrary.load_asset(asset_or_path)
    if not asset:
        _log(f"missing ability asset: {asset_or_path}")
        return

    _log(
        f"ability={asset.get_path_name()} "
        f"cost={asset.get_editor_property('initiative_cost')} "
        f"cooldown={asset.get_editor_property('cooldown')} "
        f"range={asset.get_editor_property('range')} "
        f"damage={asset.get_editor_property('base_damage')} "
        f"field_state={asset.get_editor_property('field_state')} "
        f"damage_type={asset.get_editor_property('damage_type')} "
        f"requires_char={asset.get_editor_property('requires_target_character')} "
        f"requires_tile={asset.get_editor_property('requires_target_tile')}"
    )
    _log(f" values={asset.get_editor_property('values')}")
    progression = asset.get_editor_property("skill_progression")
    if progression:
        levels = progression.get_editor_property("levels")
        _log(f" progression={progression.get_path_name()} level_count={len(levels)}")
        for index, level in enumerate(levels):
            _log(
                f"  level[{index}] title={level.title} "
                f"description={level.description} values={level.values}"
            )


for character_asset in CHARACTER_ASSETS:
    _dump_character(character_asset)

_log("done")
