import unreal


ASSET_PATH = "/Game/TopDown/Characters/Character_1/DA_Character1Progression"


def _text(value: str):
    return unreal.TextLibrary.conv_string_to_text(value)


def _load_asset(path: str):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"missing asset: {path}")
    return asset


def _save_asset(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save asset: {asset.get_path_name()}")


def _skillpoint(title: str, description: str, values: dict[str, float]):
    skillpoint = unreal.RoundSkillpointDetails()
    skillpoint.title = _text(title)
    skillpoint.description = description
    skillpoint.values = values
    return skillpoint


def _tier(title: str, choices):
    tier = unreal.RoundSkillpointTier()
    tier.title = _text(title)
    tier.choices = list(choices)
    return tier


def main():
    asset = _load_asset(ASSET_PATH)

    asset.set_editor_property("character_id", "Character1")
    asset.set_editor_property("display_name", _text("Tank"))
    asset.set_editor_property("description", _text("Character-only upgrades for Character 1."))

    tiers = [
        _tier(
            "Kettenfundament",
            [
                _skillpoint(
                    "Zusatzkette",
                    "Erhoeht MaxChains um {ChainLimitAdd}.",
                    {
                        "ChainLimitAdd": 1.0,
                    },
                ),
                _skillpoint(
                    "Ruhiger Zug",
                    "Das erste Mal pro Zug, wenn Character 1 eine Kette verbraucht, erhaelt er {FirstChainSpendInitiativeRefund} Initiative zurueck.",
                    {
                        "FirstChainSpendInitiativeRefund": 10.0,
                    },
                ),
                _skillpoint(
                    "Schwerer Stand",
                    "Hat Character 1 seinen letzten Zug ohne Bewegung beendet, erleidet er bis zu seinem naechsten Zug nur {StationaryIncomingDamageMultiplier}x Schaden.",
                    {
                        "StationaryIncomingDamageMultiplier": 0.9,
                    },
                ),
            ],
        ),
        _tier(
            "Druckaufbau",
            [
                _skillpoint(
                    "Kurzer Zyklus",
                    "Verringert ChainRechargeTime um {-ChainRechargeTimeAdd}.",
                    {
                        "ChainRechargeTimeAdd": -20.0,
                    },
                ),
                _skillpoint(
                    "Druckfeld",
                    "Gegner, die einen Debuff von Character 1 haben, verursachen gegen ihn nur {IncomingDamageMultiplierFromDebuffedEnemies}x Schaden.",
                    {
                        "IncomingDamageMultiplierFromDebuffedEnemies": 0.9,
                    },
                ),
                _skillpoint(
                    "Notglied",
                    "Beginnt Character 1 einen Zug mit 0 Ketten, erhaelt er {ZeroChainTurnStartGainChain} Kette, verliert aber {ZeroChainTurnStartInitiativePenalty} Initiative.",
                    {
                        "ZeroChainTurnStartGainChain": 1.0,
                        "ZeroChainTurnStartInitiativePenalty": 20.0,
                    },
                ),
            ],
        ),
        _tier(
            "Kettenkern",
            [
                _skillpoint(
                    "Kettenpolster",
                    "Solange Character 1 mindestens 1 Kette hat, erleidet er nur {DamageMultiplierWhileHasChain}x Schaden.",
                    {
                        "DamageMultiplierWhileHasChain": 0.85,
                    },
                ),
                _skillpoint(
                    "Ueberlauf",
                    "Wuerde Character 1 eine Kette regenerieren, waehrend er bereits am Limit ist, erhaelt er stattdessen {ChainOverflowInitiativeGain} Initiative.",
                    {
                        "ChainOverflowInitiativeGain": 12.0,
                    },
                ),
                _skillpoint(
                    "Laengerer Druck",
                    "Debuffs von Character 1 halten 25% laenger.",
                    {
                        "DebuffDurationMultiplier": 1.25,
                    },
                ),
            ],
        ),
        _tier(
            "Ankerpunkt",
            [
                _skillpoint(
                    "Standhalten",
                    "Wuerde Character 1 sterben, ueberlebt er stattdessen, verbraucht alle Ketten und heilt sich pro verbrauchter Kette um {CheatDeathHealPerConsumedChain}. Cooldown: {CheatDeathCooldownInitiative} Initiative.",
                    {
                        "CheatDeathHealPerConsumedChain": 250.0,
                        "CheatDeathCooldownInitiative": 160.0,
                    },
                ),
                _skillpoint(
                    "Eiserner Anker",
                    "Delay auf Character 1 ist nur 70% so effektiv. Wird er von einem Gegner bewegt, ignoriert er diese Bewegung mit 50% Chance.",
                    {
                        "DelayReceivedMultiplier": 0.7,
                        "IgnoreEnemyForcedMovementChance": 0.5,
                    },
                ),
                _skillpoint(
                    "Letzter Widerstand",
                    "Solange Character 1 0 Ketten hat, erhaelt er {DefenseWhileNoChain} Defense.",
                    {
                        "DefenseWhileNoChain": 40.0,
                    },
                ),
            ],
        ),
    ]

    asset.set_editor_property("tiers", tiers)
    _save_asset(asset)
    unreal.log(f"[apply_character1_progression] saved {asset.get_path_name()}")


if __name__ == "__main__":
    main()
