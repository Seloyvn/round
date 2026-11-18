import unreal


PROGRESSION_DIR = "/Game/TopDown/Data/SkillProgression"
ABILITY_DIR = "/Game/TopDown/Data/Abilities"


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


ABILITY_DEFAULTS = {
    "Knockback": {
        "display_name": "Knockback",
        "description": "Stoesst Ziele zurueck und kontrolliert Positionen.",
        "levels": [
            _skillpoint("Wucht", "Erhoeht den Schaden um {Damage}.", {"Damage": 100.0}),
            _skillpoint("Eisenstoss", "Erhoeht Knockback um {KnockbackDistance}.", {"KnockbackDistance": 1.0}),
            _skillpoint("Vorbote", "Das Ziel erleidet bis zum Ende des naechsten Zuges dieses Ziels {IncomingDamageMultiplierOnHit}x Schaden.", {"IncomingDamageMultiplierOnHit": 1.15}),
            _skillpoint("Aufprall", "Stoppt das Ziel an einem Hindernis oder Character, erleidet es zusaetzlich {WallBonusDamage} Schaden.", {"WallBonusDamage": 180.0}),
            _skillpoint("Unwucht", "Das Ziel verliert nach Knockback {TargetInitiativeLossOnHit} Initiative.", {"TargetInitiativeLossOnHit": 20.0}),
            _skillpoint("Schutzstoss", "Das Ziel verursacht bis zum Ende des naechsten Zuges dieses Ziels nur {OutgoingDamageMultiplierAfterHit}x Schaden.", {"OutgoingDamageMultiplierAfterHit": 0.9}),
            _skillpoint("Jaegerinstinkt", "Gegen bereits debuffte Ziele verursacht Knockback {DamageMultiplierVsDebuffed}x Schaden.", {"DamageMultiplierVsDebuffed": 1.2}),
            _skillpoint("Raumgewinn", "Nach Knockback darf Character 1 sich {SelfStepTowardTarget} Feld in Richtung des Ziels bewegen, ohne Initiative zu bezahlen.", {"SelfStepTowardTarget": 1.0}),
            _skillpoint("Kettenzug", "Verringert ChainCost um {-ChainCost}, erhoeht aber den Cooldown um {Cooldown}.", {"ChainCost": -1.0, "Cooldown": 40.0}),
            _skillpoint("Hartnaeckig", "Das Ziel erhaelt nach Knockback Slow {ApplySlowOnHitStrength} fuer {ApplySlowOnHitDuration} Zug.", {"ApplySlowOnHitStrength": 0.2, "ApplySlowOnHitDuration": 1.0}),
            _skillpoint("Gegenhalt", "Erleidet Character 1 vom Ziel Schaden, erleidet er fuer {SelfIncomingDamageMultiplierFromTargetDuration} Zug nur {SelfIncomingDamageMultiplierFromTarget}x Schaden.", {"SelfIncomingDamageMultiplierFromTarget": 0.8, "SelfIncomingDamageMultiplierFromTargetDuration": 1.0}),
            _skillpoint("Rammschlag", "Erhoeht den Schaden um {Damage}, erhoeht aber die Initiativekosten um {InitiativeCost}.", {"Damage": 120.0, "InitiativeCost": 10.0}),
        ],
    },
    "Delay": {
        "display_name": "Delay",
        "description": "Verzoegert Gegner und zerstoert ihren Turn-Rhythmus.",
        "levels": [
            _skillpoint("Tiefe Verzoegerung", "Erhoeht Delay um {DelayAmount}.", {"DelayAmount": 12.0}),
            _skillpoint("Stoerimpuls", "Erhoeht den Schaden um {Damage}.", {"Damage": 80.0}),
            _skillpoint("Schwungklau", "Character 1 erhaelt {SelfInitiativeGainOnUse} Initiative.", {"SelfInitiativeGainOnUse": 15.0}),
            _skillpoint("Taktbruch", "Gegen bereits debuffte Ziele erhoeht sich Delay um weitere {ExtraDelayVsDebuffed}.", {"ExtraDelayVsDebuffed": 12.0}),
            _skillpoint("Unterbrechen", "Ziele mit hoher Initiative verursachen bis zum Ende des naechsten Zuges dieses Ziels nur {OutgoingDamageMultiplierOnHighInitiativeTarget}x Schaden.", {"OutgoingDamageMultiplierOnHighInitiativeTarget": 0.8}),
            _skillpoint("Bremsfeld", "Gegner im Umkreis von {AoEInitiativeLossRadius} Feld um das Ziel verlieren {AoEInitiativeLossAmount} Initiative.", {"AoEInitiativeLossRadius": 1.0, "AoEInitiativeLossAmount": 8.0}),
            _skillpoint("Kettenriss", "Weitere Debuffs auf dem Ziel halten {IncomingEffectDurationAddOnTarget} Zug laenger.", {"IncomingEffectDurationAddOnTarget": 1.0}),
            _skillpoint("Erschwerter Zugriff", "Das Ziel erhaelt bis zum Ende des naechsten Zuges dieses Ziels +{MoveCostAddOnTarget} MoveCost.", {"MoveCostAddOnTarget": 2.0}),
            _skillpoint("Sparsame Ausfuehrung", "Verringert Initiativekosten um {-InitiativeCost}, verringert aber Delay um {-DelayAmount}.", {"InitiativeCost": -5.0, "DelayAmount": -5.0}),
            _skillpoint("Nachhall", "Wird ein bereits verzoegertes Ziel erneut von Character 1 getroffen, verliert es zusaetzlich {AdditionalInitiativeLossIfAlreadyDelayed} Initiative.", {"AdditionalInitiativeLossIfAlreadyDelayed": 10.0}),
            _skillpoint("Zielerfassung", "Das Ziel kann bis zum Ende des naechsten Zuges dieses Ziels keine Bewegungsboni erhalten.", {"BlockMovementBonusesOnTarget": 1.0}),
            _skillpoint("Kurzer Takt", "Verringert den Cooldown um {-Cooldown}, verringert aber den Schaden um {-Damage}.", {"Cooldown": -40.0, "Damage": -50.0}),
        ],
    },
    "Shackle": {
        "display_name": "Shackle",
        "description": "Haelt Gegner fest und erschwert jede Bewegung.",
        "levels": [
            _skillpoint("Schwere Fessel", "Erhoeht Slow auf {SlowStrength}.", {"SlowStrength": 0.15}),
            _skillpoint("Lange Fessel", "Erhoeht die Dauer um {SlowDuration} Zug.", {"SlowDuration": 1.0}),
            _skillpoint("Schwacher Stand", "Gefesselte Ziele verursachen nur {OutgoingDamageMultiplierWhileShackled}x Schaden.", {"OutgoingDamageMultiplierWhileShackled": 0.88}),
            _skillpoint("Bleierne Schritte", "Immer wenn sich ein gefesseltes Ziel bewegt, verliert es {InitiativeLossOnMove} Initiative.", {"InitiativeLossOnMove": 8.0}),
            _skillpoint("Ausweglos", "Gefesselte Ziele koennen keine Bewegungsboni erhalten.", {"BlockMovementBonuses": 1.0}),
            _skillpoint("Unter Druck", "Character 1 verursacht gegen gefesselte Ziele {IncomingDamageMultiplierFromCauserWhileShackled}x Schaden.", {"IncomingDamageMultiplierFromCauserWhileShackled": 1.2}),
            _skillpoint("Stoerender Griff", "Erhoeht den Schaden um {Damage}.", {"Damage": 80.0}),
            _skillpoint("Restfessel", "Wenn Shackle endet, verliert das Ziel {InitiativeLossOnExpire} Initiative.", {"InitiativeLossOnExpire": 15.0}),
            _skillpoint("Sparsame Fessel", "Verringert Initiativekosten um {-InitiativeCost}, verringert aber die Dauer um {-SlowDuration} Zug.", {"InitiativeCost": -5.0, "SlowDuration": -1.0}),
            _skillpoint("Schwere Kette", "Gegen bereits debuffte Ziele haelt Shackle {SlowDuration} Zug laenger.", {"SlowDuration": 1.0}),
            _skillpoint("Haltepunkt", "Solange Shackle aktiv ist, erleidet Character 1 vom Ziel nur {SelfIncomingDamageMultiplierFromTargetWhileShackled}x Schaden.", {"SelfIncomingDamageMultiplierFromTargetWhileShackled": 0.85}),
            _skillpoint("Kurzer Takt", "Verringert den Cooldown um {-Cooldown}, erhoeht aber die Initiativekosten um {InitiativeCost}.", {"Cooldown": -40.0, "InitiativeCost": 5.0}),
        ],
    },
    "Bind": {
        "display_name": "Bind",
        "description": "Bestrafte Bewegung mit Trigger-Schaden und Turn-Control.",
        "levels": [
            _skillpoint("Schwere Bindung", "Erhoeht den Trigger-Schaden um {Damage}.", {"Damage": 120.0}),
            _skillpoint("Tiefer Ruck", "Erhoeht den Trigger-Delay um {DelayAmount}.", {"DelayAmount": 10.0}),
            _skillpoint("Lauernde Bindung", "Erhoeht die Dauer um {DurationInitiative} Initiative.", {"DurationInitiative": 40.0}),
            _skillpoint("Widerhaken", "Wenn Bind ausloest, verursacht das Ziel bis zum Ende des naechsten Zuges dieses Ziels nur {TriggerOutgoingDamageMultiplier}x Schaden.", {"TriggerOutgoingDamageMultiplier": 0.85}),
            _skillpoint("Fester Griff", "Ein gebundenes Ziel kann keine Bewegungsboni erhalten.", {"BlockMovementBonusesOnTarget": 1.0}),
            _skillpoint("Kettenstoss", "Wenn Bind ausloest, verlieren Gegner im Umkreis von {TriggerAoEInitiativeLossRadius} Feld ebenfalls {TriggerAoEInitiativeLossAmount} Initiative.", {"TriggerAoEInitiativeLossRadius": 1.0, "TriggerAoEInitiativeLossAmount": 10.0}),
            _skillpoint("Hetzjagd", "Bereits debuffte Ziele erleiden {TriggerDamageMultiplierVsDebuffed}x Trigger-Schaden.", {"TriggerDamageMultiplierVsDebuffed": 1.2}),
            _skillpoint("Jagdzeichen", "Wenn Bind ausloest, erhaelt Character 1 {TriggerInitiativeGainForCauser} Initiative.", {"TriggerInitiativeGainForCauser": 15.0}),
            _skillpoint("Sparsame Falle", "Verringert Initiativekosten um {-InitiativeCost}, verringert aber den Trigger-Schaden um {-Damage}.", {"InitiativeCost": -5.0, "Damage": -60.0}),
            _skillpoint("Schwere Fesselung", "Wenn Bind ausloest, erhaelt das Ziel Slow {TriggerSlowAmount} fuer {TriggerSlowDuration} Zug.", {"TriggerSlowAmount": 0.2, "TriggerSlowDuration": 1.0}),
            _skillpoint("Nachbeben", "Der Trigger verlangsamt auch die Umgebung und haelt {DurationInitiative} Initiative laenger.", {"DurationInitiative": 40.0, "TriggerAoEInitiativeLossRadius": 1.0, "TriggerAoEInitiativeLossAmount": 6.0}),
            _skillpoint("Kurzer Takt", "Verringert den Cooldown um {-Cooldown}, verringert aber die Dauer um {-DurationInitiative} Initiative.", {"Cooldown": -40.0, "DurationInitiative": -40.0}),
        ],
    },
    "Taunt": {
        "display_name": "Taunt",
        "description": "Zwingt Gegner auf Character 1 und bestraft falsche Ziele.",
        "levels": [
            _skillpoint("Tiefer Spott", "Erhoeht den Schadensmalus gegen andere Ziele auf {ReductionAmount}.", {"ReductionAmount": 0.15}),
            _skillpoint("Langer Spott", "Erhoeht die Dauer um {DurationInitiative} Initiative.", {"DurationInitiative": 40.0}),
            _skillpoint("Einschuechterung", "Das Ziel verliert sofort {ImmediateInitiativeLoss} Initiative.", {"ImmediateInitiativeLoss": 15.0}),
            _skillpoint("Offene Deckung", "Solange Taunt aktiv ist, erleidet das Ziel von Character 1 {IncomingDamageMultiplierFromCauserWhileTaunted}x Schaden.", {"IncomingDamageMultiplierFromCauserWhileTaunted": 1.15}),
            _skillpoint("Verunsicherung", "Greift das Ziel jemand anderen an, verliert es danach {InitiativeLossWhenIgnoringCauser} Initiative.", {"InitiativeLossWhenIgnoringCauser": 15.0}),
            _skillpoint("Drohkulisse", "Bereits debuffte Ziele bleiben {ExtraDurationVsDebuffed} Zug laenger provoziert.", {"ExtraDurationVsDebuffed": 1.0}),
            _skillpoint("Standhalten", "Solange Taunt aktiv ist, erleidet Character 1 vom Ziel nur {SelfIncomingDamageMultiplierFromTargetWhileTaunted}x Schaden.", {"SelfIncomingDamageMultiplierFromTargetWhileTaunted": 0.85}),
            _skillpoint("Dominanz", "Greift das Ziel Character 1 an, erhaelt Character 1 {InitiativeGainForCauserWhenAttacked} Initiative.", {"InitiativeGainForCauserWhenAttacked": 10.0}),
            _skillpoint("Sparsame Provokation", "Verringert Initiativekosten um {-InitiativeCost}, verringert aber die Dauer um {-DurationInitiative} Initiative.", {"InitiativeCost": -5.0, "DurationInitiative": -40.0}),
            _skillpoint("Blossstellung", "Solange Taunt aktiv ist, erleidet das Ziel {IncomingDamageMultiplierWhileTaunted}x Schaden.", {"IncomingDamageMultiplierWhileTaunted": 1.1}),
            _skillpoint("Taktischer Druck", "Erhoeht den direkten Schaden um {Damage}.", {"Damage": 80.0}),
            _skillpoint("Kurzer Takt", "Verringert den Cooldown um {-Cooldown}, der direkte Schaden entfaellt aber.", {"Cooldown": -40.0, "Damage": -80.0}),
        ],
    },
}


def main():
    for skill_key, payload in ABILITY_DEFAULTS.items():
        progression = _load_asset(f"{PROGRESSION_DIR}/DA_SkillProgression_{skill_key}")
        ability = _load_asset(f"{ABILITY_DIR}/DA_Ability_{skill_key}")

        progression.set_editor_property("skill_id", ability.get_editor_property("ability_id"))
        progression.set_editor_property("display_name", _text(payload["display_name"]))
        progression.set_editor_property("description", _text(payload["description"]))
        progression.set_editor_property("levels", payload["levels"])
        _save_asset(progression)

        ability.set_editor_property("skill_progression", progression)
        _save_asset(ability)

        unreal.log(f"[apply_character1_ability_progressions] saved {skill_key}")


if __name__ == "__main__":
    main()
