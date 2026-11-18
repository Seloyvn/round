# Round Game Mechanics Overview

## Spielidee

Round ist ein rundenbasiertes Taktikspiel auf einem Grid. Zwei Teams treten gegeneinander an. Jeder Character hat eigene Werte, Fähigkeiten, eine Klassenressource oder Klassenpassive und kann im Match über Positionierung, Terrain, Initiative-Management und Effekte beeinflusst werden.

Das Spiel ist so gebaut, dass sich Kämpfe nicht nur über rohen Schaden entscheiden. Wichtig sind auch:

- Wer wann am Zug ist
- Wie teuer Bewegung und Aktionen sind
- Welche Felder kontrolliert oder bedroht werden
- Welche Debuffs oder vorbereiteten Zustände bereits auf dem Feld liegen
- Welche Rolle ein Character im Team erfüllen soll

## Grundstruktur eines Zuges

Ein Character wird nicht einfach in einer festen Reihenfolge aktiviert, sondern über Initiative.

Wichtige Werte:

- `CurrentInitiative`
- `MaxInitiative`
- `TurnDelay`
- `AttackCost`
- `MoveCost`

Ein Character sammelt über die Zeit Initiative, bis er wieder am Zug ist. Aktionen und Bewegung verbrauchen Initiative oder verändern den Abstand bis zum nächsten Zug. Dadurch entsteht ein Spielrhythmus, in dem schnelle Characters öfter handeln können, während schwere Characters härter zuschlagen oder mehr aushalten.

Im Zug kann ein Character:

- sich bewegen
- normal angreifen
- eine Fähigkeit nutzen
- den Zug beenden

## Bewegung

Bewegung läuft über Grid-Felder.

Jedes Feld kann Bewegungskosten beeinflussen. Zusätzlich hat jeder Character einen eigenen `MoveCost`. Wie teuer ein Schritt ist, hängt also vom Character und vom Terrain ab.

Dadurch ist Bewegung nicht nur Reichweite, sondern auch Ressourcenverbrauch:

- Mobile Characters können leichter Druck aufbauen oder sich lösen
- Langsame Characters committen sich stärker in ihre Position
- Terrain kann Wege günstig oder teuer machen

## Normale Angriffe

Jeder Character hat einen normalen Angriff mit:

- `Attack`
- `AttackRange`
- `AttackCost`

Normale Angriffe sind nicht nur ein Fallback. Je nach Character und Build können sie Teil des eigentlichen Plans sein, zum Beispiel:

- zum Combo-Aufbau
- zum Blut-Aufbau
- für sichere günstige Turns
- als Follow-up nach Setup durch Fähigkeiten oder Passive

## Fähigkeiten

Fähigkeiten sind data-driven über Ability-DataAssets gebaut. Jede Fähigkeit hat im Kern:

- Anzeige- und Identitätsdaten
- Werte wie Schaden, Reichweite, Kosten, Cooldown und Sonderwerte
- eine Child-Klasse mit ihrer konkreten Logik
- optional eine Skill-Progression für Fähigkeits-Skillpunkte

Die eigentliche Speziallogik liegt nicht mehr im Ability-Monolithen, sondern soll in den jeweiligen Ability-Child-Klassen liegen.

Fähigkeiten können zum Beispiel:

- Schaden machen
- Gegner verschieben
- Initiative verändern
- Debuffs anwenden
- Trigger auf Feldern erzeugen
- Terrain oder Kontrollzonen verändern

## Initiative und Turn Control

Initiative ist eine der zentralen Ressourcen des Spiels.

Sie bestimmt:

- wie nah ein Character am nächsten Zug ist
- wie teuer manche Aktionsfolgen sind
- wie stark bestimmte Fähigkeiten oder Passive skalieren
- wie gut ein Team gegnerische Pläne verzögern oder beschleunigen kann

Es gibt mehrere Arten, Initiative zu beeinflussen:

- direkte Initiativekosten von Aktionen
- `Delay`-Effekte
- `TurnDelay`
- Passive Effekte, die Initiative zurückgeben oder entziehen

Dadurch ist Turn Control ein eigenes taktisches Feld neben Schaden und Positionierung.

## Effekte und Debuffs

Effekte sind eigene Actor-/Child-Klassen. Sie liegen auf Characters und verändern für eine Dauer oder bis zu einem Trigger das Verhalten.

Beispiele:

- `Slow`
- `Taunt`
- `Invisibility`
- `Bind`
- `PuppetDefend`
- `Bleed`
- Markierungs- oder Kontroll-Effekte

Wichtige Arten von Effekten:

- Zeitbasierte Effekte
- Trigger-Effekte, die bei Bewegung, Schaden oder Zugstart auslösen
- Buffs auf den eigenen Character
- Debuffs auf Gegner

Effekte sind wichtig, weil viele Fähigkeiten nicht als einmaliger Schadensknopf gedacht sind, sondern spätere Entscheidungen beeinflussen.

## Terrain

Terrain ist ein echter Bestandteil des Kampfs.

Die Terrain-Logik liegt in eigenen Terrain-Child-Klassen. Das erlaubt unterschiedliche Regeln pro Feldtyp.

Aktuelle Beispiele:

- `Forest`
  reduziert eingehenden Schaden auf diesem Feld
- `Street`
  reduziert effektive Bewegungskosten
- `Rubble`
  erhöht Bewegungskosten
- `Fire`
  verursacht Schaden beim Zugstart auf dem Feld
- `Rocks`
  blockiert Bewegung
- `Wall`
  blockiert Bewegung und Sicht

Terrain soll nicht nur Hintergrund sein, sondern Positionsentscheidungen verändern.

## Vision

Characters und Teams haben Vision.

Vision bestimmt unter anderem:

- welche gegnerischen Characters sichtbar sind
- was ausgewählt oder sicher angesteuert werden kann
- welche UI-Elemente oder Klickpfade verfügbar sind

Vision ist deshalb nicht nur kosmetisch, sondern ein Teil von Informationskontrolle und Sicherheit.

## Team- und Charakteridentität

Jeder Character hat:

- Basiswerte
- eine Rollenidee
- Fähigkeiten
- eine Character-Passive oder Character-Ressource
- Character-Skillpunkte
- später zusätzlich ausdefinierte Fähigkeits-Skillpunkte

Die Character-Skillpunkte verändern den Character selbst, nicht einzelne Fähigkeiten. Dazu gehören zum Beispiel:

- Basiswerte
- Klassenressourcen
- allgemeine Passive-Effekte
- Klassenidentität

Die Fähigkeits-Skillpunkte dagegen sollen später einzelne Fähigkeiten ausbauen.

## Character-Skillpunkte

Character-Skillpunkte werden im Hauptmenü vergeben.

Wichtige Eigenschaften:

- Punkte gelten pro Character, nicht teamweit
- pro Character gibt es ein eigenes Character-Progression-Asset
- das Asset ist in Tiers mit Choices gegliedert
- pro Reihe wird genau eine Option gewählt
- die gewählten Einträge wirken zur Laufzeit auf Stats, Ressourcen und Passive

Das System ist bewusst so aufgebaut, dass eine Reihe unterschiedliche Entscheidungen anbietet, statt nur drei stärkere oder schwächere Zahlenvarianten derselben Art.

## Fähigkeits-Skillpunkte

Für Fähigkeiten existiert bereits die Grundstruktur:

- jede Ability kann eine `SkillProgression` haben
- das Hauptmenü kann solche Progressionen anzeigen und speichern

Der nächste große Schritt ist die inhaltliche Ausgestaltung dieser Fähigkeits-Skillpunkte pro Ability. Ziel ist, dass auch dort pro Reihe echte Richtungsentscheidungen entstehen, zum Beispiel:

- mehr Kontrolle
- mehr Zuverlässigkeit
- mehr Effizienz
- mehr offensive Wucht

## Aktueller Designansatz

Das Spiel profitiert stark davon, wenn Characters nicht nur eine starre Rolle haben. Gute Builds sollen:

- das Team im Menü unterschiedlich ausrichten
- im Match unterschiedliche Antworten erlauben
- aber trotzdem die Kernidentität des Characters erhalten

Ein Character sollte sich durch Skillpunkte also nicht komplett fremd anfühlen, sondern besser in einer bestimmten Richtung funktionieren:

- sicherer
- aggressiver
- flexibler
- kontrollierender
- ressourceneffizienter

## Zusammenfassung

Round ist ein Taktikspiel, in dem mehrere Systeme gleichzeitig ineinandergreifen:

- Initiative
- Bewegung
- Terrain
- Sicht
- Fähigkeiten
- Effekte
- Character-Passiven
- Character- und Fähigkeits-Skillpunkte

Die interessantesten Entscheidungen entstehen dort, wo mehrere dieser Systeme gleichzeitig relevant sind. Gute Spielzüge sind deshalb meist nicht einfach nur der größte Schadenswert, sondern:

- der richtige Character
- zur richtigen Zeit
- auf dem richtigen Feld
- mit dem richtigen Setup
- gegen das richtige Ziel

