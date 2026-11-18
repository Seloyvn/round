import unreal


ASSET_PATH = "/Game/TopDown/Data/SkillProgression/DA_SkillProgression_PiercingShot"


def point(title, description, values):
    item = unreal.RoundSkillpointDetails()
    item.title = unreal.TextLibrary.conv_string_to_text(title)
    item.description = description
    item.values = values
    return item


asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not asset:
    raise RuntimeError(f"Missing asset: {ASSET_PATH}")

asset.set_editor_property("skill_id", "PiercingShot")
asset.set_editor_property("display_name", unreal.TextLibrary.conv_string_to_text("PiercingShot"))
asset.set_editor_property("description", unreal.TextLibrary.conv_string_to_text("Linienangriff fuer Druck auf mehrere Ziele und harte Durchbrueche."))
asset.set_editor_property("levels", [
    point("Panzerbohrer", "Alle getroffenen Ziele werden behandelt, als haetten sie {DefenseIgnore} weniger Defense.", {"DefenseIgnore": 20.0}),
    point("Schockmunition", "Jeder getroffene Charakter verliert {InitiativeLossOnHit} Initiative.", {"InitiativeLossOnHit": 10.0}),
    point("Splitterkern", "Wird ein Feldobjekt zerstoert, erleiden angrenzende Gegner {DestroyedObjectSplashDamageMultiplier}x Schaden. Jeder Gegner kann pro Schuss nur einmal davon getroffen werden.", {"DestroyedObjectSplashDamageMultiplier": 0.4}),
    point("Wuchtlinie", "Das erste getroffene Ziel wird um {KnockbackFirstTargetDistance} Feld zurueckgestossen, wenn das Feld frei ist.", {"KnockbackFirstTargetDistance": 1.0}),
    point("Kreuzungspunkt", "Der zweite und jeder weitere getroffene Charakter erleidet {LaterTargetDamageMultiplier}x Schaden.", {"LaterTargetDamageMultiplier": 1.2}),
    point("Langlauf", "Erhoeht die Reichweite um {Range}.", {"Range": 2.0}),
    point("Verkettete Wunden", "Getroffene Gegner werden markiert. Der naechste normale Angriff auf einen markierten Gegner verursacht {ChainMarkDamageMultiplier}x Angriffsschaden an allen anderen markierten Gegnern und entfernt alle Markierungen.", {"ChainMarkDamageMultiplier": 0.3}),
    point("Umlenkgeschoss", "Nach dem ersten Treffer aendert das Projektil seine Richtung zum naechsten Gegner innerhalb der verbleibenden Reichweite.", {"RedirectAfterFirstHit": 1.0}),
    point("Zielspur", "Getroffene Gegner werden fuer {TargetMarkDurationInitiative} Initiative markiert. Normale Angriffe von Character 2 erhalten gegen sie {AttackRangeVsMarked} Reichweite.", {"TargetMarkDurationInitiative": 80.0, "AttackRangeVsMarked": 2.0}),
    point("Doppelladung", "Wenn nach dem Schuss Ammo verbleibt, wird dieselbe Schussbahn mit {RepeatShotDamageMultiplier}x Schaden erneut abgefeuert. Kostet 1 Ammo, aber keine Initiative.", {"RepeatShotDamageMultiplier": 0.5}),
    point("Endloser Durchschuss", "Jeder Treffer an einem Charakter oder zerstoerbaren Objekt setzt die verbleibende Reichweite des Projektils auf die volle Reichweite zurueck.", {"ResetRangeOnEveryHit": 1.0}),
    point("Panzerbrechende Vorbereitung", "Jeder Treffer erzeugt einen Stack. Der naechste normale Angriff verbraucht alle Stacks und reduziert die Defense des Ziels pro Stack um {DefenseReductionPerHitStack} fuer {DefenseReductionDurationInitiative} Initiative.", {"DefenseReductionPerHitStack": 5.0, "DefenseReductionDurationInitiative": 80.0}),
])

if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save asset: {ASSET_PATH}")
