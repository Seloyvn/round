import unreal


ASSET_PATH = "/Game/TopDown/Characters/Character_2/DA_Character2Progression"


def _text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def _skillpoint(title, description, values):
    point = unreal.RoundSkillpointDetails()
    point.title = _text(title)
    point.description = description
    point.values = values
    return point


def _tier(title, choices):
    tier = unreal.RoundSkillpointTier()
    tier.title = _text(title)
    tier.choices = choices
    return tier


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    raise RuntimeError(f"Missing asset: {ASSET_PATH}")

asset.set_editor_property("character_id", "Character2")
asset.set_editor_property("display_name", _text("Sniper"))
asset.set_editor_property("description", _text("Character-only upgrades for Character 2."))
asset.set_editor_property("tiers", [
    _tier("Ausrichtung", [
        _skillpoint("Hartgeschoss", "Erhoeht Attack um {AttackAdd}, aber auch MoveCost um {MoveCostAdd}.", {"AttackAdd": 40.0, "MoveCostAdd": 2.0}),
        _skillpoint("Zusatztasche", "Erhoeht MaxAmmo um {MaxAmmoAdd}.", {"MaxAmmoAdd": 1.0}),
        _skillpoint("Distanzpanzerung", "Erleidet gegen Gegner in mehr als einem Feld Entfernung nur {RangedDamageMultiplier}x Schaden.", {"RangedDamageMultiplier": 0.85}),
    ]),
    _tier("Positionierung", [
        _skillpoint("Sparschuss", "Der erste normale Angriff pro Zug verbraucht keine Ammo.", {"FirstBasicAttackFreeAmmo": 1.0}),
        _skillpoint("Leichte Ausruestung", "Verringert MoveCost um {-MoveCostAdd}, aber auch MaxAmmo um {-MaxAmmoAdd}.", {"MoveCostAdd": -3.0, "MaxAmmoAdd": -1.0}),
        _skillpoint("Sicherer Stand", "Wenn Character 2 sich seit seinem vorherigen Zug nicht bewegt hat, erhaelt er {DefenseAddWhileHoldingPosition} Defense.", {"DefenseAddWhileHoldingPosition": 25.0}),
    ]),
    _tier("Munitionsrhythmus", [
        _skillpoint("Schnellladen", "Erhoeht AmmoRegen um {AmmoRegenAdd}.", {"AmmoRegenAdd": 1.0}),
        _skillpoint("Notfallmagazin", "Beginnt Character 2 einen Zug mit 0 Ammo, wird Ammo auf voll gesetzt, verliert aber {RefillAmmoInitiativePenalty} Initiative.", {"RefillAmmoOnEmptyTurnStart": 1.0, "RefillAmmoInitiativePenalty": 25.0}),
        _skillpoint("Letzter Schuss", "Faellt Character 2 durch einen Schuss zum ersten Mal in einem Zug auf 0 Ammo, erhaelt er {EmptyMagazineInitiativeGain} Initiative.", {"EmptyMagazineInitiativeGain": 10.0}),
    ]),
    _tier("Scharfschuetze", [
        _skillpoint("Eingeschossen", "Jeder Munitionsschuss ohne Bewegung gibt einen Stapel Eingeschossen. Jeder Stapel erhoeht den Schaden weiterer Schuesse um {StationaryShotDamagePerStack}x, maximal {StationaryShotMaxStacks} Stapel. Bewegung entfernt alle Stapel.", {"StationaryShotDamagePerStack": 0.1, "StationaryShotMaxStacks": 3.0}),
        _skillpoint("Eine im Lauf", "Solange Character 2 genau 1 Ammo hat, verursacht er {AllDamageMultiplierWhileOneAmmo}x Schaden. Besiegt ein solcher Schuss ein Ziel, erhaelt er einmal pro Zug 1 Ammo zurueck.", {"AllDamageMultiplierWhileOneAmmo": 1.2, "LastAmmoKillRefund": 1.0}),
        _skillpoint("Vorbereitet", "Beginnt Character 2 seinen Zug mit voller Ammo, werden die Cooldowns seiner munitionsverbrauchenden Faehigkeiten um {FullAmmoTurnStartAbilityCooldownReduction} Initiative reduziert.", {"FullAmmoTurnStartAbilityCooldownReduction": 20.0}),
    ]),
])

if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save asset: {ASSET_PATH}")

unreal.log(f"[apply_character2_progression] saved {asset.get_path_name()}")
