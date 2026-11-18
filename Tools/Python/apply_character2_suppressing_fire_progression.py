import unreal

ASSET_PATH = "/Game/TopDown/Data/SkillProgression/DA_SkillProgression_SuppressingFire"

def point(title, description, values):
    item = unreal.RoundSkillpointDetails()
    item.title = unreal.TextLibrary.conv_string_to_text(title)
    item.description = description
    item.values = values
    return item

asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    raise RuntimeError(f"Missing asset: {ASSET_PATH}")

asset.set_editor_property("skill_id", "SuppressingFire")
asset.set_editor_property("display_name", unreal.TextLibrary.conv_string_to_text("SuppressingFire"))
asset.set_editor_property("description", unreal.TextLibrary.conv_string_to_text("Unterdrueckt ein Ziel und eroeffnet unterschiedliche Eigen-, Team- und Kontrollbuilds."))
asset.set_editor_property("levels", [
    point("Schweres Feuer", "Suppressing Fire verursacht {Damage} zusaetzlichen Schaden.", {"Damage": 70.0}),
    point("Niederhalten", "Der naechste normale Angriff des Ziels kostet {BasicAttackCostAdd} zusaetzliche Initiative.", {"BasicAttackCostAdd": 15.0}),
    point("Markiermunition", "Das Ziel bleibt fuer das gesamte Team sichtbar und kann waehrend der Unterdrueckung nicht unsichtbar werden.", {"RevealToAllTeams": 1.0}),
    point("Feuerleitung", "Der erste Treffer jedes verbuendeten Charakters verlaengert die Unterdrueckung um {AllyHitDurationExtension} Initiative.", {"AllyHitDurationExtension": 10.0}),
    point("Offene Deckung", "Reduziert die Defense des unterdrueckten Ziels um {DefenseReduction}.", {"DefenseReduction": 15.0}),
    point("Kreuzfeuer", "Nach dem ersten Treffer eines Verbuendeten kostet Character 2s naechster normaler Angriff gegen das Ziel keine Ammo.", {"FreeFollowupAfterAllyHit": 1.0}),
    point("Nachfuehren", "Bewegt sich das Ziel, wird der Cooldown von Suppressing Fire einmalig um {CooldownReductionOnMove} Initiative reduziert.", {"CooldownReductionOnMove": 30.0}),
    point("Bestaetigter Abschuss", "Stirbt das Ziel waehrend der Unterdrueckung, erhaelt Character 2 {AmmoOnKill} Ammo.", {"AmmoOnKill": 1.0}),
    point("Vorbelastetes Ziel", "Gegen ein bereits debufftes Ziel verbraucht Suppressing Fire keine Ammo.", {"FreeAmmoVsDebuffed": 1.0}),
    point("Unterdrueckungskette", "Stirbt das Ziel, springt die Unterdrueckung mit verbleibender Dauer auf den naechsten Gegner innerhalb von {ChainRange} Feldern ueber.", {"ChainOnKill": 1.0, "ChainRange": 2.0}),
    point("Ueberwachungsfeuer", "Bei der ersten Bewegung des Ziels feuert Character 2 einen normalen Angriff mit {ReactionDamageMultiplier}x Schaden. Kostet 1 Ammo und kann kritisch treffen.", {"ReactionDamageMultiplier": 0.5}),
    point("Kopf unten", "Das Ziel kann sich nicht bewegen, bis es erneut Schaden von einem Charakter erleidet. Danach bleibt der normale Bewegungskosten-Malus bestehen.", {"RootUntilDamaged": 1.0}),
])

if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save asset: {ASSET_PATH}")
