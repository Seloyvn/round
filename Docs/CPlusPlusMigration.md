# Round C++ Migration

Dieses Projekt war bisher ein reines Blueprint-Projekt. Die neue C++-Basis liegt jetzt unter `Source/Round`.

## Bereits angelegt

- Unreal-C++-Modul `Round`
- Basisklassen fuer Match, Character, Ability, Effect, Grid und Teamdaten
- Datenklassen fuer Charaktere und Faehigkeiten

## Zielarchitektur

- C++ enthaelt alle Spielregeln und den autoritativen Zustand.
- Blueprints bleiben fuer UI, VFX, Audio, Animation und konkrete Asset-Zusammenstellung.
- Balancing-Daten sollen langfristig in `PrimaryDataAsset` oder `DataTable` liegen.

## Neue C++-Basisklassen

- `URoundGameInstance`
- `ARoundGameMode`
- `ARoundGameState`
- `ARoundCharacter`
- `ARoundAbilityActor`
- `ARoundEffectActor`
- `ARoundGridTile`
- `URoundAttributeSetComponent`
- `URoundCharacterData`
- `URoundAbilityData`

## Empfohlene Reparenting-Reihenfolge

1. `BP_Game_Instance` auf `URoundGameInstance`
2. `BP_TopDownGameMode` auf `ARoundGameMode`
3. `BP_GameState` auf `ARoundGameState`
4. `BP_Character` auf `ARoundCharacter`
5. Skill-Basis `BP_Ability` auf `ARoundAbilityActor`
6. Effekt-Basis `BP_Effect` auf `ARoundEffectActor`
7. Feld-/Hex-Basis `Field` oder `BP_Terrain` auf `ARoundGridTile`

## Wichtige Migrationsregel

Nicht sofort alle Child-Blueprints umbauen. Erst die Basisklasse reparenten, dann die bisherige Blueprint-Logik Schritt fuer Schritt nach C++ verschieben und nach jedem Schritt im Editor testen.

## Naechste sinnvolle C++-Pakete

### Paket 1: Kernkampf

- Basisangriff
- Schaden, Defense, Heilung
- Initiative ausgeben und regenerieren
- Turnwechsel ueber `ARoundGameState`

### Paket 2: Grid und Bewegung

- Nachbarfelder
- Reichweitenberechnung
- Pfadsuche
- Sichtlinie
- Terrain-Kosten und Blocker

### Paket 3: Skillsystem

- Zielpruefung
- Cooldowns
- Kosten
- Effekte anwenden
- Projektil-/Tile-/Character-Targeting

### Paket 4: Team und Save-Domain

- Teamvalidierung
- Team-Slots
- Serialisierung fuer Saves
- Mapping der bestehenden Menu-Blueprints auf C++-Daten

## Was noch offen ist

Die bestehende Blueprint-Logik in den `.uasset`-Graphs wurde nicht 1:1 uebernommen. Ohne Unreal Editor lassen sich die Node-Graphen nur teilweise lesen. Die neue C++-Struktur ist daher bewusst eine tragfaehige Zielarchitektur, auf die die vorhandenen Blueprints nun migriert werden koennen.
