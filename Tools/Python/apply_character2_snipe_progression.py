import unreal


ASSET_PATH = "/Game/TopDown/Data/SkillProgression/DA_SkillProgression_Snipe"


def point(title, description, values):
    item = unreal.RoundSkillpointDetails()
    item.title = unreal.TextLibrary.conv_string_to_text(title)
    item.description = description
    item.values = values
    return item


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    raise RuntimeError(f"Missing asset: {ASSET_PATH}")

asset.set_editor_property("skill_id", "Snipe")
asset.set_editor_property("display_name", unreal.TextLibrary.conv_string_to_text("Snipe"))
asset.set_editor_property("description", unreal.TextLibrary.conv_string_to_text("Praeziser Fernschuss fuer Initiativekontrolle und Picks."))
asset.set_editor_property("levels", [
    point("Aufgerissene Deckung", "Snipe reduziert die Defense des Ziels fuer {DefenseReductionDurationInitiative} Initiative um {DefenseReductionOnHit}.", {"DefenseReductionOnHit": 20.0, "DefenseReductionDurationInitiative": 80.0}),
    point("Tiefer Schuss", "Erhoeht den Schaden pro fehlender Initiative um {InitativeScaling}.", {"InitativeScaling": 2.0}),
    point("Zielmarkierung", "Der naechste Treffer von Character 2 verursacht {MarkNextHitDamageMultiplier}x Schaden und verbraucht die Markierung.", {"MarkNextHitDamageMultiplier": 1.15, "MarkDurationInitiative": 80.0}),
    point("Freie Linie", "Ziele ohne benachbarte Verbuendete erleiden {DamageMultiplierVsIsolated}x Schaden.", {"DamageMultiplierVsIsolated": 1.2}),
    point("Durchschuss", "Ein Gegner direkt hinter dem Ziel erleidet {SecondaryTargetDamageMultiplier}x Snipe-Schaden.", {"SecondaryTargetDamageMultiplier": 0.6}),
    point("Distanzmesser", "Ein Treffer auf maximaler Reichweite erstattet einmal pro Zug die verbrauchte Ammo.", {"RefundAmmoAtMaxRange": 1.0}),
    point("Sauberer Treffer", "Besiegt Snipe das Ziel, wird die verbrauchte Ammo erstattet.", {"RefundAmmoOnKill": 1.0}),
    point("Nadelpunkt", "Fehlen dem Ziel mindestens {RequiredMissingInitiativeForDefenseIgnore} Initiative, ignoriert Snipe {ConditionalDefenseIgnore} Defense.", {"RequiredMissingInitiativeForDefenseIgnore": 40.0, "ConditionalDefenseIgnore": 25.0}),
    point("Kammerdruck", "Wird Snipe mit der letzten Ammo abgefeuert, wird das Ziel um {LastAmmoKnockbackDistance} Feld zurueckgestossen.", {"LastAmmoKnockbackDistance": 1.0}),
    point("Nachbeben", "Trifft Snipe ein bereits debufftes Ziel, wird dessen aeltester entfernbarer Debuff um {ExtendDebuffDurationInitiative} Initiative beziehungsweise einen Zug verlaengert.", {"ExtendDebuffDurationInitiative": 40.0}),
    point("Hinrichtung", "Ziele unter {LowHealthThresholdPercent}% Leben erleiden {DamageMultiplierVsLowHealth}x Schaden. Ein Kill reduziert den Cooldown einmal pro Zug um {CooldownReductionOnKill}.", {"LowHealthThresholdPercent": 35.0, "DamageMultiplierVsLowHealth": 1.25, "CooldownReductionOnKill": 40.0}),
    point("Kalter Lauf", "Solange Snipe einsatzbereit ist, erhaelt er pro {ReadyDamageInitiativeInterval} vergangener Initiative {ReadyDamagePercentPerInterval}x Schaden, maximal {ReadyDamagePercentCap}x.", {"ReadyDamageInitiativeInterval": 3.0, "ReadyDamagePercentPerInterval": 0.01, "ReadyDamagePercentCap": 0.3}),
])

if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save asset: {ASSET_PATH}")
