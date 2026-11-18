import unreal


DESCRIPTIONS = {
    "Knockback": "Fügt einem angrenzenden Gegner 150 Schaden zu und stößt ihn 3 Felder zurück.",
    "Delay": "Fügt einem angrenzenden Gegner 50 Schaden zu und verzögert seinen nächsten Zug um 30 Initiative.",
    "Shackle": "Verlangsamt einen Gegner in bis zu 2 Feldern Entfernung für 70 Initiative.",
    "Bind": "Bindet einen Gegner für 100 Initiative. Bewegt er sich, erleidet er 100 Schaden und verliert 20 Initiative.",
    "Taunt": "Verspottet einen Gegner für 70 Initiative. Er verursacht 20% weniger Schaden gegen andere Ziele.",

    "PiercingShot": "Feuert durch eine Linie von bis zu 5 Feldern und fügt allen getroffenen Gegnern 200 Schaden zu.",
    "Snipe": "Fügt einem Gegner in bis zu 5 Feldern Entfernung Schaden zu. Der Schaden steigt mit der seit dem letzten Zug vergangenen Initiative.",
    "Concentrate": "Verstärkt den nächsten verursachten Schaden abhängig von der bis zum Treffer vergangenen Initiative.",
    "Aim": "Markiert ein Feld in bis zu 5 Feldern Entfernung und fügt dem Zielgebiet nach einer Verzögerung 400 Schaden zu.",
    "SuppressingFire": "Fügt einem Gegner 140 Schaden zu und erhöht seine Bewegungskosten für 40 Initiative um 2.",

    "Shadowstrike": "Fügt einem Gegner in 3 bis 4 Feldern Entfernung 200 Schaden zu und erzeugt 2 Combo-Punkte.",
    "Armorpen": "Fügt einem angrenzenden Gegner 200 Schaden zu, ignoriert 30 Verteidigung und erzeugt 1 Combo-Punkt.",
    "Execute": "Fügt einem angrenzenden Gegner 100 Schaden plus 50 Schaden pro Combo-Punkt zu und verbraucht die Combo-Punkte.",
    "HookShot": "Zieht Character 3 auf ein freies Feld in bis zu 4 Feldern Entfernung.",
    "DeathMark": "Markiert einen Gegner für 80 Initiative. Character 3 verursacht 15% mehr Schaden gegen dieses Ziel.",

    "Fireball": "Fügt Gegnern im Zielgebiet mit Radius 1 insgesamt 400 Feuerschaden zu.",
    "Lightningstrike": "Fügt einem Ziel in bis zu 4 Feldern Entfernung 200 Blitzschaden zu.",
    "Rockblast": "Fügt einem Ziel in bis zu 4 Feldern Entfernung 300 physischen Schaden zu.",
    "Icebeam": "Fügt einem Gegner 250 Eisschaden zu und verlangsamt ihn für 50 Initiative um 50%.",
    "ArcaneShift": "Teleportiert den Magier auf ein freies Feld in bis zu 3 Feldern Entfernung.",

    "Dash": "Bewegt den Charakter durch einen Gegner und fügt ihm 200 Schaden zu.",
    "Decimate": "Fügt einem angrenzenden Gegner 200 Schaden plus 10% seines maximalen Lebens zu.",
    "Heal": "Verbraucht gesammeltes Blut und heilt den Charakter um 50 plus die verbrauchte Blutmenge.",
    "Rend": "Fügt einem angrenzenden Gegner 170 Schaden zu und lässt ihn für 80 Initiative bluten.",
    "SpinToWin": "Fügt allen angrenzenden Gegnern 400 Schaden zu.",

    "LaunchBlade": "Fügt einem Ziel in bis zu 4 Feldern Entfernung 100 Schaden zu und platziert dort eine Klinge.",
    "DriveBlades": "Bewegt alle aktiven Klingen durch einen gewählten Zielpunkt. Jede Klinge fügt getroffenen Gegnern 150 Schaden zu.",
    "BladeSwap": "Tauscht die Position des Bladecallers mit einer Klinge. Die zurückfliegende Klinge fügt Gegnern in ihrer Linie 200 Schaden zu.",
    "BladeCall": "Bewegt die dem Ziel nächste Klinge zum Zielpunkt und fügt Gegnern im Radius 1 150 Schaden zu.",
    "RecallBlade": "Zieht eine Klinge zum Bladecaller zurück, fügt Gegnern in ihrer Flugbahn 150 Schaden zu und nimmt die Klinge als Schutzstapel auf.",

    "DelayHeal": "Markiert einen Verbündeten in bis zu 3 Feldern Entfernung und heilt ihn nach 25 Initiative um 300 Leben.",
    "Pulse": "Fügt allen Gegnern neben einem gewählten Verbündeten 200 Schaden zu und stößt sie 1 Feld von ihm weg.",
    "Rescue": "Zieht einen Verbündeten in bis zu 4 Feldern Entfernung auf ein freies Feld neben den Cleric.",
    "Shield": "Gewährt einem Verbündeten für 65 Initiative einen Schild über 350 Schaden.",
    "Cleanse": "Entfernt einen zufälligen entfernbaren Debuff von einem Verbündeten.",

    "Turret": "Platziert für 200 Initiative ein Geschütz. Es schießt alle 50 Initiative auf Gegner in Reichweite und verursacht 130 Schaden.",
    "Mine": "Platziert eine Mine, die sich nach 30 Initiative schärft und Gegnern im Radius 1 250 Schaden zufügt.",
    "ShieldEmitter": "Platziert für 200 Initiative einen Emitter. Er gewährt Verbündeten im Radius 2 regelmäßig einen Schild über 100 Schaden.",
    "SpotterBeacon": "Platziert für 240 Initiative einen Peilsender. Gegner im Radius 3 erleiden für 45 Initiative 15% mehr Schaden.",
    "Overload": "Löst alle eigenen Geräte in bis zu 4 Feldern Entfernung sofort manuell aus.",

    "Accelerate": "Reduziert die aktuellen Fähigkeits-Cooldowns eines Verbündeten um 50 Initiative.",
    "BorrowedTime": "Verschiebt für 100 Initiative 45% des Schadens eines Verbündeten. Der gespeicherte Schaden wird nach Ablauf zugefügt.",
    "Stasis": "Versetzt ein Ziel für 70 Initiative in Stasis. Es ist immun, kann aber keine Initiative erzeugen.",
    "TemporalAnchor": "Speichert Position und Leben eines Verbündeten für 110 Initiative und setzt ihn nach Ablauf dorthin zurück.",
    "TimeSiphon": "Entzieht einem Gegner 35 Initiative und erzeugt 2 Zeitfragmente.",

    "ThornPatch": "Verwandelt ein Feld in Dornen. Gegner auf Dornen erleiden 120 Naturschaden.",
    "Grove": "Verwandelt ein Feld in einen Hain, der Verbündete auf diesem Feld unterstützt.",
    "Rootsnare": "Fügt einem Gegner 100 Schaden zu und hindert ihn für 45 Initiative an der Bewegung. Auf Naturfeldern hält die Wurzel 35 Initiative länger.",
    "WildBloom": "Löst alle verbundenen Naturfelder aus: Dornen verursachen 160 Schaden, Haine heilen 140 Leben und Wälder geben 15 Initiative.",
    "BearForm": "Verwandelt den Druiden in einen Bären mit eigenen Kampfwerten. Die Fähigkeit wird in Bärenform zur Rückverwandlung.",

    "Lunge": "Stürmt bis zu 5 Felder in einer Linie vor und fügt dem ersten getroffenen Gegner 170 Schaden zu.",
    "Overshoot": "Springt über einen Gegner auf ein freies Feld hinter ihm und fügt ihm 210 Schaden zu.",
    "ExecuteDive": "Springt zu einem Gegner und fügt ihm 150 Schaden zu. Ziele unter 35% Leben erleiden 80% mehr Schaden.",
    "HoldtheLine": "Nimmt eine Abwehrhaltung ein und fügt Gegnern 170 Schaden zu, wenn sie die Angriffsreichweite des Dragoons verlassen.",
    "Repel": "Fügt einem Gegner in bis zu 2 Feldern Entfernung 130 Schaden zu und stößt ihn 1 Feld zurück.",

    "StoneWall": "Erschafft eine 3 Felder lange Steinmauer an der gewählten Position.",
    "Faultline": "Fügt Gegnern in einer Linie bis zu 4 Felder weit 120 Schaden zu. Felsen in der Linie erhöhen den Schaden um 110.",
    "Detonate": "Sprengt einen gewählten Felsen und fügt Gegnern im Radius 1 130 Schaden zu.",
    "Tremor": "Fügt Gegnern im Radius 1 um den Geomancer 110 Schaden zu.",
    "Landsplitter": "Spaltet einen breiten Bereich bis zu 4 Felder weit, fügt Gegnern 160 Schaden zu, markiert sie und stößt sie zurück.",

    "QuickDraw": "Spielt die älteste Handkarte gegen einen Gegner. Rang und Farbe bestimmen Schaden und Zusatzeffekt.",
    "DoubleDraw": "Zieht 2 Karten vom Deck auf die Hand.",
    "LowStakes": "Wirft alle Handkarten unter Rang 10 ab und zieht für jede abgeworfene Karte eine neue Karte.",
    "Flush": "Spielt alle Karten der häufigsten Farbe. Herz heilt, Pik verursacht Schaden, Kreuz verursacht Schaden und Initiativeverlust, Karo gibt Initiative.",
    "RigTheDeck": "Wählt eine Handkarte und verwandelt sie entsprechend dem aktuellen Deckprofil.",

    "Smite": "Fügt einem angrenzenden Gegner 300 heiligen Schaden zu.",
    "Condem": "Fügt einem Gegner 120 heiligen Schaden zu und verstummt ihn für 60 Initiative. Das Ziel muss kürzlich Schaden verursacht haben, den die Paladinaura reduziert hat.",
    "SanctifyGround": "Weiht das Feld des Paladins und fügt Gegnern im Radius 1 für 70 Initiative bei jedem Initiative-Schritt 5 heiligen Schaden zu.",
    "Bulwark": "Reduziert für 120 Initiative den erlittenen Schaden um 25% und erhöht den verursachten Schaden um 50%.",
    "ShieldOath": "Schützt einen Verbündeten für 120 Initiative und leitet 30% seines eingehenden Schadens auf den Paladin um.",
}


updated = 0
missing = []
for asset_name, description in DESCRIPTIONS.items():
    path = f"/Game/TopDown/Data/Abilities/DA_Ability_{asset_name}"
    ability = unreal.EditorAssetLibrary.load_asset(path)
    if not ability:
        missing.append(asset_name)
        continue

    ability.set_editor_property("description", description)
    if not unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {ability.get_path_name()}")
    updated += 1

if missing:
    raise RuntimeError("Missing ability assets: " + ", ".join(missing))

unreal.log(f"[apply_functional_ability_descriptions] Updated {updated} abilities")
