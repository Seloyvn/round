import unreal


ASSET_PATH = "/Game/TopDown/Characters/Gambler/DA_GamblerProgression"


def text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def skill(title, description, values):
    result = unreal.RoundSkillpointDetails()
    result.title = text(title)
    result.description = description
    result.values = values
    return result


def tier(title, choices):
    result = unreal.RoundSkillpointTier()
    result.title = text(title)
    result.choices = choices
    return result


def main():
    asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not asset:
        raise RuntimeError(f"missing asset: {ASSET_PATH}")
    asset.set_editor_property("character_id", "Gambler")
    asset.set_editor_property("display_name", text("Gambler"))
    asset.set_editor_property("description", text("Card economy, calculated risks and flexible tempo."))
    asset.set_editor_property("tiers", [
        tier("Opening Hand", [
            skill("Deep Pockets", "Increases the maximum hand size by {MaxHandCards}.", {"MaxHandCards": 1.0}),
            skill("Hot Hand", "Draws {StartingHandCardsAdd} additional card at match start.", {"StartingHandCardsAdd": 1.0}),
            skill("Quick Step", "Reduces movement cost by {-MoveCostAdd}.", {"MoveCostAdd": -1.0}),
        ]),
        tier("The Deal", [
            skill("Fast Dealer", "Draws {TurnStartDrawAdd} additional card at turn start.", {"TurnStartDrawAdd": 1.0}),
            skill("Patient Player", "Draws {IdleTurnDrawAdd} additional card after ending a turn without acting.", {"IdleTurnDrawAdd": 1.0}),
            skill("Deadeye", "Increases Attack by {AttackAdd}.", {"AttackAdd": 75.0}),
        ]),
        tier("Raise the Stakes", [
            skill("Loaded Deck", "Removes ranks below {MinimumDeckRankAdd}+2 from the deck.", {"MinimumDeckRankAdd": 2.0}),
            skill("High Roller", "Adds {CardRankMultiplierAdd} to card rank multipliers.", {"CardRankMultiplierAdd": 0.15}),
            skill("Insurance", "Increases maximum health by {MaxHealthAdd}.", {"MaxHealthAdd": 200.0}),
        ]),
        tier("All In", [
            skill("Royal Hand", "Increases maximum hand size and card scaling.", {"MaxHandCards": 1.0, "CardRankMultiplierAdd": 0.10}),
            skill("Tempo Play", "Reduces turn delay and increases maximum initiative.", {"TurnDelayAdd": -10.0, "MaxInitiativeAdd": 10.0}),
            skill("Poker Face", "Reduces all incoming damage to {AllDamageMultiplier}x.", {"AllDamageMultiplier": 0.85}),
        ]),
    ])
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save asset: {ASSET_PATH}")
    unreal.log(f"GAMBLER_PROGRESSION saved {asset.get_path_name()}")


if __name__ == "__main__":
    main()
