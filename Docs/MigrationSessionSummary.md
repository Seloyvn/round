# Migration Session Summary

Wir bauen das Unreal-Projekt `C:\Users\marco\Documents\Unreal Projects\round` schrittweise von Blueprint-Logik auf C++ um, direkt im bestehenden Projekt. Das Projekt war urspruenglich fast vollstaendig Blueprint-basiert. Inzwischen gibt es ein C++-Modul `Round`, und die wichtigsten Blueprint-Basisklassen wurden bereits auf C++ gereparentet.

## Wichtige Dateien

- `C:\Users\marco\Documents\Unreal Projects\round\Round.uproject`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Game\RoundGameState.h`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Private\Game\RoundGameState.cpp`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Game\RoundGameInstance.h`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Private\Game\RoundGameInstance.cpp`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Actors\RoundCharacter.h`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Actors\RoundGridTile.h`
- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Actors\RoundTeamActor.h`
- `C:\Users\marco\Documents\Unreal Projects\round\Docs\CPlusPlusMigration.md`
- `C:\Users\marco\Documents\Unreal Projects\round\Content\TopDown\Characters\Character_1\DA_Character1Progression.uasset`

## Arbeitsweise

- Die Blueprints werden blockweise nach C++ portiert.
- Frueher wurde dafuer `export.txt` genutzt.
- Inzwischen gibt es einen deutlich besseren Voll-Export unter:
  - `C:\Users\marco\Documents\Unreal Projects\round\export`
- Dort liegen `.T3D`-Exporte fast aller relevanten Blueprints, Structs, Enums und Data Assets.
- Diese `.T3D`-Dateien sind jetzt die Hauptquelle fuer die Rekonstruktion der originalen Blueprint-Logik.
- Fuer Character- und Ability-Progressionen bleiben die eigentlichen angezeigten Skillpoint-Titel und -Beschreibungen in den jeweiligen Progression-Assets die Source of Truth.
- C++-Defaultfunktionen koennen diese Assets befuellen oder initialisieren, aber die sichtbaren Namen und Beschreibungstexte sollen als Asset-Inhalt verstanden werden, nicht als fest verdrahtete Runtime-Texte.

## Relevante Exporte

Besonders wichtig fuer den weiteren Umbau:

- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\BP_GameState.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\BP_Team.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\BPL_Team.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\Blueprints\BP_Game_Instance.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\BP_Saves.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\DT_Format.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\Enums\E_Format.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\Enums\E_Character.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\Characters\BP_Character.T3D`
- `C:\Users\marco\Documents\Unreal Projects\round\export\Game\TopDown\Field\Field.T3D`

## Bereits erledigt

### 1. C++-Projektbasis

- Das `Round`-Modul wurde angelegt.
- Build-Probleme fuer UE 5.3 wurden behoben.
- Das Projekt kompiliert wieder.
- Die relevanten Blueprint-Klassen wurden auf die neuen C++-Basisklassen gereparentet.

### 2. BP_GameState Block 1: Feld- und Turn-Basis

Aus `BP_GameState` wurden bereits diese Funktionen nach C++ uebertragen:

- `GetField`
- `GetFieldAt`
- `GetFields`
- `CharacterDie`
- `SetNextTurn`

In `ARoundGameState` wurden dafuer umgesetzt:

- `Fields`
- `RegisterField`
- `GetFields()`
- `GetField()`
- `GetFieldAt()`
- `CharacterDie()`
- `SetNextTurn()`
- `CurrentTurnCharacter`
- `OnTurnChanged`
- `OnBoardStateChanged`

Zusaetzlich in `ARoundCharacter`:

- `CurrentTile`
- `CanGetTurn()`
- `InitiativeToTurn()`
- `BeginTurn()`

Zusaetzlich in `ARoundGridTile`:

- `GridPosition` als `FVector2D`

### 3. BP_GameState Block 2: Freie Felder und Vision

Portiert wurden:

- `Get Close Free Field`
- `Get Close Free FieldFrom`
- `SetVision`

Dafuer ergaenzt:

- `ARoundGridTile::GetHexDistanceTo()`
- `ARoundGameState::GetCloseFreeField()`
- `ARoundGameState::GetCloseFreeFieldFrom()`
- `ARoundGameState::SetVision()`

Wichtig bei `SetVision()`:

- laeuft nur auf Authority/Server
- ruft aktuell teils noch bestehende Blueprint-Funktionen auf:
  - `SetVisionFields` auf Team-Objekten
  - `UpdateVision` auf `Player1` und `Player2`

### 4. BP_GameState Block 3: Spawn- und Team-Aufbau-Basis

Portiert wurden:

- `SpawnCharacter`
- `SpawnTrap`
- `CreateTeams`

In `ARoundGameState` umgesetzt:

- `SpawnCharacter(TSubclassOf<ARoundCharacter>, FVector2D, UObject*, int64, FName)`
- `SpawnTrap(TSubclassOf<AActor>, int32, ARoundGridTile*, ARoundCharacter*)`
- `CreateTeams()`

### 5. Terrain- und Setup-Basis

Es gibt jetzt in `ARoundGameState` bereits:

- `OrderTerrainTypes(...)`
- `SpawnFieldAt(...)`
- `CreateTerrain(...)`
- `CreateFieldFromParameters(...)`
- `BeginPlay()`-Setup
- `TerrainTypes`
- `bFinishedGenerating`

Zusaetzlich in `ARoundGridTile`:

- `GetDistance(...)`
- `GetDistanceFields(...)`
- `SetTerrainType(...)`
- `TerrainObject`
- `TerrainVisualClasses`

### 6. Erste native Structs und GameInstance-Domain

In `RoundTypes.h` und `URoundGameInstance` gibt es jetzt echte C++-Typen fuer die alten Blueprint-Daten:

- `FRoundFormatDefinition`
- `FRoundTeamDefinition`
- `FRoundCharacterEntry`
- `FRoundSkillEntry`
- weitere Detail-Structs
- `StrTeam`
- `FormatDefinitions`
- `TryGetFormatDefinition(...)`

Damit kann `BeginPlay()` im `GameState` bereits einen nativen Format-Pfad aus der `GameInstance` nutzen.

### 7. Erste Team-Basis

Es gibt jetzt eine neue C++-Klasse:

- `C:\Users\marco\Documents\Unreal Projects\round\Source\Round\Public\Actors\RoundTeamActor.h`

Mit erster Logik fuer:

- `GetRoundGameState()`
- `SetVisionFields()`
- `SpawnCharacters(...)`
- `VisionFields`
- `TeamId`
- `TeamDefinition`

Ausserdem in `ARoundGameState`:

- `SpawnCharacterByRowName(...)`
- `CharacterRowClasses`

Damit ist der erste native Ersatz fuer `BP_Team` angelegt.

### 8. Save-, Format- und Team-Domain weitergezogen

Zusaetzlich umgesetzt:

- `ERoundFormat` hat jetzt echte native Werte fuer `3-16` und `5-25`
- `URoundGameInstance::Init()`
- native Standard-Initialisierung fuer `FormatDefinitions`
- natives Laden von `Saves` ueber `LoadGameFromSlot`
- Reflection-basierte Uebersetzung von `BP_Saves.Teams` nach `LoadedTeams`
- `StrTeam` wird beim Start bevorzugt aus `LoadedTeams[0]` gesetzt
- `ARoundGameState::BeginPlay()` nutzt Formatdefinitionen nur dann direkt fuer die Feldgenerierung, wenn sie schon vollstaendige Boarddaten enthalten
- sonst faellt das Setup sauber auf die bisherigen `Initial*`-Daten bzw. vorhandenen Board-Defaults zurueck
- `ARoundGameState::CreateTeams()` nutzt jetzt bevorzugt `LoadedTeams[0]` und `LoadedTeams[1]`
- `ARoundTeamActor::SpawnCharacters()` verwendet jetzt die echten `Skillpoints` aus `Str_Character`
- `ARoundGameState::SpawnCharacterByRowName()` ist robuster und gleicht Namensvarianten wie `Character1`, `Character_1`, `BP_Character_1` und `Charcter1` gegen Editor-Mappings ab

### 9. Build-Stand

- Der aktuelle Stand kompiliert erfolgreich als `RoundEditor Win64 Development` mit UE 5.3.

### 10. Character-Block weitergezogen

In `ARoundCharacter` gibt es jetzt zusaetzlich native Gameplay-Funktionen fuer:

- `AttackField(...)`
- `GetRoundGameState()`
- `EnemyField(...)`
- `CanMoveTo(...)`
- `GetValueFromKey(...)`
- `GetCritChance()`
- `GetCritModifier()`
- `GetAttackCost()`
- `GetAttackRange()`
- `GetVisionFields()`
- `HasVision(...)`
- `DamageCharacter(...)`
- `ApplyRoundDamage(...)`
- `EndTurn()`
- `Die()`

Der aktuelle Stand bildet damit bereits einen guten Teil der einfachen Character-Helfer aus `BP_Character` nativ ab:

- Turn-Ende triggert Effect-EndTurn-Hooks und setzt den naechsten Zug
- Sichtfelder kommen direkt aus `CurrentTile + VisionRange`
- Feindfeld- und Bewegbarkeitspruefung laufen nativ
- `CharacterData` hat jetzt eine native `Details`-Map fuer Werte wie `AttackCost`, `Range`, `CritChance`, `CritModifier` oder `PercentArmorPen`
- `HasVision(...)` beruecksichtigt jetzt auch Effect-Hooks ueber `SetHasVision`
- der native `AttackField`-Pfad prueft Ziel, Reichweite, Sicht und Initiativekosten
- `DamageCharacter(...)` nutzt jetzt Crit, `PercentArmorPen` und Effect-Hooks ueber `OnDealDamage`
- `ApplyRoundDamage(...)` bildet die Blueprint-Verteidigungsformel als `Amount * 100 / (100 + max(Defense - Ignore, 0))` nach
- Tod meldet sich nativ an den `GameState`

Zusaetzlich erweitert:

- `URoundAttributeSetComponent::ApplyFinalDamage(...)`
- `ARoundEffectActor::SetHasVision(...)`
- `ARoundEffectActor::OnDealDamage(...)`

## Noch nicht fertig

- `CreateTeams()` ist noch nicht die vollstaendige alte Blueprint-Version.
- Die genauen DataTable-Row-Werte aus `DT_Format` sind noch nicht sauber als nativer Vollersatz rekonstruiert.
- `BPL_Team.GetFormat` ist inhaltlich noch nicht als kompletter C++-Pfad nachgebaut; aktuell gibt es dafuer eine native Basis mit Standardformaten und Save-Ladevorgang.
- `BP_Team` ist erst als erste C++-Basis vorhanden, nicht vollstaendig 1:1 migriert.
- Der Damage-Pfad aus `BP_Character.DamageCharacter/TakeDamage` ist jetzt als erste native Version vorhanden, aber `DamageType`-spezifische Spezialfaelle und weitere Effect-Hooks sind noch nicht vollstaendig 1:1 uebernommen.
- Charakter-, Ability- und UI-Details liegen weiterhin groesstenteils noch in Blueprints.

## Naechster sinnvoller Block

Die naechste starke Ausbaustufe ist jetzt:

- `BP_Team` weiter vertiefen
- `BPL_Team` selektive Hilfsfunktionen nativ abbilden
- `DT_Format` bzw. die echten Format-Row-Werte rekonstruieren
- `BP_Character` weiter vertiefen, vor allem `GetMoveCost`, Ziel-/Movement-Checks, zusätzliche Effect-Hooks und Ability-Anbindung
- danach `BP_Ability`

Damit kann der komplette Pfad

- Teamdaten laden
- Format ableiten
- Feldlayout erzeugen
- Charaktere spawnen

## Aktueller Skill-Stand

Der native Ability-Kern in `ARoundAbilityActor` deckt inzwischen diese Skills direkt in C++ ab:

- `Heal`
- `Concentrate`
- `Snipe`
- `PiercingShot`
- `Armorpen`
- `Fireball`
- `Execute`
- `Lightningstrike`
- `Icebeam`
- `Rockblast`
- `Decimate`
- `SpinToWin`
- `Dash`
- `Delay`
- `Knockback`

### 11. Character-5-Skillblock nativ

Zusaetzlich wurden jetzt die Character-5-Skills nativ in `ARoundAbilityActor::ExecuteAbility(...)` umgesetzt:

- `Decimate`
  - Schaden = `GetAbilityDamage() + Target.MaxHealth * PercentDamage`
- `SpinToWin`
  - trifft alle gegnerischen Charaktere in `GetAbilityRange()` um den Anwender
- `Dash`
  - nutzt `GetCloseFreeFieldFrom(Target, Current)` und setzt den Charakter nativ auf das gefundene Feld

Zusaetzlich in `ARoundCharacter`:

- native `SetField(...)`-Funktion als C++-Ersatz fuer den alten Blueprint-Move-/Reposition-Pfad
- `MoveToTile(...)` verwendet jetzt intern ebenfalls `SetField(...)`

### 12. Character-1-Skills teilweise nativ

Zusaetzlich wurden jetzt erste Character-1-Skills nativ in `ARoundAbilityActor::ExecuteAbility(...)` umgesetzt:

- `Delay`
  - nutzt den bestehenden Delay-Pfad auf dem Zielcharakter
  - faegt normalen Ability-Schaden hinzu
  - beruecksichtigt die vorhandene Chain-Logik des Character-1-Pfads
- `Knockback`
  - fuegt Schaden zu
  - bewegt das Ziel anschliessend ueber `GetCloseFreeFieldFrom(...)` von der Quelle weg
  - beruecksichtigt ebenfalls die vorhandene Chain-Logik

Die Character-1-spezifischen Chain-Funktionen bleiben in diesem Schritt bewusst noch ueber Reflection an die bestehenden Blueprint-Character-Klassen angebunden. So liegt der Skill-Ablauf bereits in C++, ohne dass die Character-1-Sondermechanik sofort komplett umgebaut werden musste.

### 13. Weiterer Fuenfer-Skillblock nativ

Zusaetzlich wurden jetzt weitere fuenf Skills nativ in `ARoundAbilityActor::ExecuteAbility(...)` umgesetzt:

- `HookShot`
  - zieht den Anwender ueber `GetCloseFreeFieldFrom(Target, CurrentField)` an das Zielfeld heran
- `Taunt`
  - haengt einen nativen `Taunt`-Effekt an das Ziel
  - der Effekt reduziert ausgehenden Schaden gegen andere Ziele, aber nicht gegen den `Causer`
- `Shackle`
  - haengt einen nativen `Slow`-Effekt mit `Amount` und `SlowDuration` an das Ziel
- `PuppetMove`
  - loest das Puppet nativ auf und bewegt es ueber `MoveToTile(...)`
- `PuppetSwitch`
  - tauscht die Felder von Charakter und Puppet ueber `SetField(...)`

Zusaetzlich in `ARoundEffectActor`:

- neue `CauserCharacter`-Referenz fuer Effekte mit Verursacher-Bezug
- `Taunt` ist jetzt nativ in `OnDealDamage(...)` abgebildet

### 14. Restliche Skills nativ angebunden

Der verbleibende Skill-Restblock ist jetzt ebenfalls nativ in `ARoundAbilityActor::ExecuteAbility(...)` abgebildet:

- `Aim`
  - als nativer offensiver Buff ueber einen `Aim`-Effekt
- `Bind`
  - als native Kernwirkung auf dem Zielfeld mit Delay-/Chain-Logik
- `Shadowstrike`
  - repositioniert den Anwender nahe am Ziel, verursacht Schaden, gibt Combo-Punkte und haengt `Invisibility` an
- `PuppetHit`
  - Schaden ueber das Puppet als Quelle
- `PuppetSlash`
  - Linienangriff ueber das Puppet
- `PuppetDefend`
  - nativer defensiver Puppet-Effekt mit `DamageReduction` und `Duration`

Zusaetzlich in `ARoundEffectActor` und `ARoundCharacter`:

- `Invisibility` wird jetzt nativ ueber `SetHasVision(...)` und automatisches Entfernen nach eigenem Schadensausteilen abgebildet
- `Aim` wird als naechster Schadens-Buff nativ ueber die Effektbasis abgefangen
- `PuppetDefend` nutzt jetzt einen nativen Incoming-Damage-Hook

### 15. Cleanup der C++-Architektur

Nach der Skill-Migration wurde ein erster struktureller Cleanup gemacht, damit die C++-Basis nicht nur funktional, sondern auch wartbarer wird:

- `ARoundCharacter` hat jetzt zentrale native Helper fuer Spezialmechaniken:
  - `TryHasChain(...)`
  - `ConsumeChains(...)`
  - `GainChains(...)`
  - `TryGetComboPoints()`
  - `TryConsumeComboPoints()`
  - `TryGainComboPoints(...)`
  - `TryConsumeBlood()`
  - `TryGetPuppetCharacter()`
  - `ApplyDelay(...)`
- Die Skilllogik in `ARoundAbilityActor::ExecuteAbility(...)` nutzt diese Character-Helper jetzt direkt statt verstreuter Reflection-Lambdas.
- `ARoundEffectActor` hat jetzt zusaetzlich `OnReceiveDamage(...)`, damit defensive Effekte wie `PuppetDefend` sauber in die native Schadenspipeline eingreifen koennen.
- `ARoundCharacter::GetMoveCost()` ruft Effekt-Movecost jetzt direkt ueber `Effect->GetMovecost(...)` auf statt per `FindFunction(...)`.
- `ARoundGameState::SetVision()` ruft `SetVisionFields()` fuer native `ARoundTeamActor` jetzt direkt auf; nur die verbleibenden Player-/UI-Bruecken laufen dort noch ueber Reflection.

Wichtige Details:

- `Execute` skaliert jetzt nativ ueber `DamagePerCombopoint` mit den Combo-Punkten des Character-3-Pfads und verbraucht diese danach ueber `UseComboPoints()`.
- `Lightningstrike` nutzt jetzt denselben nativen Linienpfad wie `PiercingShot` ueber `ARoundGameState::GetFieldsBetween(...)`, trifft aber nur gegnerische Charaktere entlang der Linie.
- `Icebeam` verursacht jetzt nativ Schaden und haengt einen `Slow`-Effekt an das Ziel, der ueber `GetMovecost(...)` die Bewegungskosten erhoeht.
- `Rockblast` verursacht Schaden auf dem Zielfeld und setzt zusaetzlich ein freies Feld nahe dem Ziel auf `Rocks`.

noch sauberer von Blueprint nach C++ gezogen werden.

## Update 2026-03-24 Native Spezialklassen

- Fuer die vier staerksten Charakter-Sondermechaniken gibt es jetzt echte C++-Subklassen:
  - [RoundCharacter1.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter1.h) / [RoundCharacter1.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter1.cpp)
  - [RoundCharacter3.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter3.h) / [RoundCharacter3.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter3.cpp)
  - [RoundCharacter5.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter5.h) / [RoundCharacter5.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter5.cpp)
  - [RoundCharacter6.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter6.h) / [RoundCharacter6.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter6.cpp)
- `ARoundCharacter1` kapselt Chains jetzt nativ ueber `GetChainLimit()`, `TryHasChain(...)`, `ConsumeChains(...)`, `GainChains(...)` und die native Recharge-Logik.
- `ARoundCharacter3` kapselt Combo-Punkte jetzt nativ ueber `GetMaxComboPoints()`, `GetComboPointsStart()`, Initialisierung beim Spawn und sauberes Clamping.
- `ARoundCharacter5` kapselt Blood jetzt nativ ueber `GetCurrentBlood()`, `GainBlood(...)` und `TryConsumeBlood()`.
- `ARoundCharacter6` kapselt die Puppet-Referenz jetzt nativ ueber `SetPuppetCharacter(...)` und `TryGetPuppetCharacter()`.
- Der naechste Editor-Schritt ist, `BP_Character_1`, `BP_Character_3`, `BP_Character_5` und `BP_Character_6` auf diese Klassen zu reparenten.

## Update 2026-03-24 Native Puppet-Erzeugung

- [RoundCharacter6.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter6.h) und [RoundCharacter6.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter6.cpp) erzeugen das Puppet jetzt nativ beim `BeginPlay`, wenn `bSpawnPuppetOnBeginPlay` aktiv ist.
- Der native Pfad nutzt:
  - `PuppetRowName = "Character6Puppet"`
  - `CurrentTile->GridPosition`
  - `ARoundGameState::SpawnCharacterByRowName(...)`
- Falls vorhanden, werden alte Blueprint-Properties weiterhin kompatibel gesetzt:
  - Team-Referenz ueber reflektierte `Team_0`-/`Team`-Property am Controller-Charakter
  - Rueckverweis ueber reflektierte `Controller_0`-Property am Puppet
- Damit ist der zentrale `ReceiveBeginPlay`-Pfad von `BP_Character_6` gameplayseitig nicht mehr nur Blueprint.

## Update 2026-03-24 Native Klassen fuer Character 2 und 4

- [RoundCharacter2.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter2.h) und [RoundCharacter2.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter2.cpp) kapseln die Ammo-Mechanik jetzt nativ:
  - `Ammo`
  - `bAttackedThisTurn`
  - `GetMaxAmmo()`
  - `HasAmmo()`
  - `ConsumeAmmo()`
  - `RegenerateAmmo()`
  - automatisches `RegenerateAmmo(1)` zu Turn-Beginn
  - Ammo-Verbrauch beim erfolgreichen Basisangriff
- [RoundCharacter4.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter4.h) und [RoundCharacter4.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter4.cpp) kapseln die Element-/Damage-State-Logik jetzt nativ:
  - `bAbilityUsed`
  - `DamageDealt`
  - `DamageTypes`
  - `DamageIncrease`
  - `RegisterAbilityDamageType(...)`
  - nativer Hook in `UseAbilityIndex(...)`
  - nativer Basic-Attack-Pfad ueber `DamageDealt`
  - nativer Damage-Bonus ueber `DamageIncrease`

## Update 2026-03-24 Native Terrain-Abfrage

- [RoundGridTile.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundGridTile.h) und [RoundGridTile.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundGridTile.cpp) haben jetzt native Terrain-Helfer:
  - `IsMoveable()`
  - `GetTerrainMoveCost(int32 BaseCost)`
- [RoundCharacter.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter.cpp) nutzt diese Helfer jetzt direkt in `CanMoveTo(...)` und `GetMoveCost()`.
- Damit ist der bisherige Reflection-Pfad ueber alte Terrain-Blueprint-Funktionen `GetMoveable` und `GetMovecost` aus einem zentralen Gameplay-Pfad entfernt.

## Update 2026-03-24 Native Basisklasse fuer Spezialmechaniken

- [RoundCharacter.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter.cpp) nutzt fuer Chain, Combo, Blood, Puppet und Delay jetzt keine Blueprint-Fallbacks mehr.
- Die Basismethoden laufen jetzt rein nativ:
  - `TryHasChain(...)`
  - `ConsumeChains(...)`
  - `GainChains(...)`
  - `TryGetComboPoints()`
  - `TryConsumeComboPoints()`
  - `TryGainComboPoints(...)`
  - `TryConsumeBlood()`
  - `TryGetPuppetCharacter()`
  - `ApplyDelay(...)`
- Die dedizierten Character-Klassen 1, 3, 5 und 6 bleiben fuer ihre Speziallogik die eigentlichen nativen Erweiterungspunkte.

## Update 2026-03-24 Native Player-/Team-Bruecke

- [RoundPlayerPawn.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Game/RoundPlayerPawn.h) und [RoundPlayerPawn.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Game/RoundPlayerPawn.cpp) fuehren jetzt eine native Basis fuer `BP_PlayerPawn` ein.
- Die Klasse kapselt:
  - `Team`
  - `TeamDefinition`
  - `TeamActor`
  - Blueprint-Hooks `UpdateVision`, `RefreshUI` und `TeamWin`
- [RoundGameState.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Game/RoundGameState.cpp) nutzt fuer `Player1` und `Player2` jetzt bevorzugt diese native Basis statt blindem `ProcessEvent`.
- Beim Team-Spawn werden native Player-Pawns jetzt direkt mit `TeamA` bzw. `TeamB` verbunden.
- [RoundCharacter.h](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Public/Actors/RoundCharacter.h) und [RoundCharacter.cpp](/C:/Users/marco/Documents/Unreal%20Projects/round/Source/Round/Private/Actors/RoundCharacter.cpp) haben jetzt ausserdem eine echte `OwningTeamActor`-Referenz.
- Character 6 nutzt diese Team-Referenz jetzt bevorzugt fuer sein Puppet-Spawning; nur der Rueckverweis `Controller_0` am alten Puppet-Blueprint bleibt noch als Kompatibilitaets-Bridge reflektiert.

## Was in einem neuen Chat gesagt werden sollte

Kurzfassung:

"Wir migrieren im Projekt `C:\Users\marco\Documents\Unreal Projects\round` Blueprint-Logik nach C++. Das C++-Modul ist angelegt, die Blueprints sind gereparentet, und ein grosser Teil von `BP_GameState`, Terrain-Setup, Team-/Save-/Format-Basis sowie erste `BP_Team`-Logik wurden bereits nach C++ uebertragen. Die Hauptquelle fuer die Original-Logik ist der `.T3D`-Export unter `C:\Users\marco\Documents\Unreal Projects\round\export`. Bitte arbeite auf dieser Basis weiter."

## Update 2026-03-24 Skill-Migration

- `ARoundAbilityActor` hat jetzt einen nativen `ExecuteAbility(...)`-Pfad, damit einzelne Skills gezielt in C++ abgebildet werden koennen.
- Die erste echte Skill-Migration ist umgesetzt: `Heal` wird nativ erkannt und fuehrt jetzt `BaseHeal + UseBlood() * HealFromBlood` aus.
- `UseBlood()` bleibt dabei kompatibel ueber den bestehenden Character-Blueprint aufrufbar, bis Charakter 5 vollstaendig nativ ist.
- `ARoundCharacter` hat jetzt eine native `Heal(int32)`-Funktion.
- `ARoundCharacter::GrantDefaultAbilities()` uebernimmt jetzt auch `AbilityId -> RowName`, damit weitere Skills stabiler identifiziert werden koennen.
- Verifiziert mit UE 5.3 Build: `RoundEditor Win64 Development` kompiliert erfolgreich.

## Update 2026-03-24 Weitere Skill-Migration

- `Concentrate` ist jetzt nativ in `ARoundAbilityActor::ExecuteAbility(...)` umgesetzt.
- `Concentrate` erzeugt einen nativen `ARoundEffectActor` mit:
  - `EffectId = "Concentrate"`
  - `Amount = CurrentInitiative * DamagePerInitative`
  - `RemainingTurns = CurrentInitiative + 1`
- `ARoundEffectActor::OnDealDamage(...)` bildet den alten `BP_Effect_Concentrate`-Pfad jetzt nativ ab: der naechste Schaden wird mit `1 + Amount` multipliziert und der Effekt entfernt sich danach selbst.
- `ARoundEffectActor` hat jetzt zusaetzlich:
  - `Amount`
  - `OwningCharacter`
  - `ApplyToCharacter(...)`
  - `RemoveEffect()`
- `ARoundCharacter` hat jetzt native Effektverwaltung ueber:
  - `AddEffect(...)`
  - `RemoveEffect(...)`
- `Snipe` ist nativ umgesetzt und nutzt:
  - `GetAbilityDamage() + InitativeScaling * TargetInitiative`
- `PiercingShot` ist nativ umgesetzt und nutzt jetzt eine echte C++-Linienabfrage statt Blueprint:
  - `ARoundGameState::GetFieldsBetween(...)`
  - Hex-Linie ueber Cube-Interpolation
  - trifft alle gegnerischen Charaktere auf der Linie zwischen Start- und Zielfeld
- `ARoundGameState` hat dafuer jetzt die neue wiederverwendbare Hilfsfunktion:
  - `GetFieldsBetween(ARoundGridTile* FromField, ARoundGridTile* ToField, bool bIncludeEndpoints = true)`
- Verifiziert mit UE 5.3 Build: `RoundEditor Win64 Development` kompiliert weiterhin erfolgreich.
