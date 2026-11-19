# RoundCore Migration Plan

## Objective

Move all deterministic gameplay state and rules into a standalone C++17 library named `RoundCore`. The library must compile and run without Unreal Engine. Unreal becomes an adapter for editor-authored data, input, networking, Actors, UI, audio, VFX, and animation. Training uses the same commands and rule implementation without constructing a `UWorld` or any Actors.

This is a migration of the real game, not a second simplified simulation.

## Invariants

1. There is one implementation of every migrated rule.
2. `RoundCore` has no Unreal headers, types, reflection macros, object references, file paths, rendering, networking, or wall-clock dependencies.
3. All balance and design values remain editor-authored and are exported into versioned Core definitions.
4. Commands use stable match IDs and board coordinates, never Actor names or pointers.
5. Every state-changing command returns deterministic events and an explicit result.
6. The same setup, seed, and command sequence must produce the same state hash and events.
7. A rule is removed from Actor execution only after parity tests pass.
8. Actors mirror Core state and present Core events; presentation completion never changes the gameplay result.

## Target layout

```text
RoundCore/
  include/round/core/   Public standard-C++ API
  src/                  Rule implementation
  tests/                Standalone deterministic tests
  CMakeLists.txt        Standalone library and test build

Source/Round/
  Unreal adapters, asset export, networking, Actors and presentation

Tools/Training/
  Python environment, self-play, evaluation and model tooling
```

## Core contracts

### Definitions

Immutable match content exported from Unreal:

- character definitions and resolved skill selections
- ability definitions and additive values
- terrain definitions
- effect definitions
- format, objective, and map definitions
- schema and content version identifiers

### State

Mutable state required to resume a match exactly:

- board and occupancy
- characters, teams, health, initiative, positions, cooldowns, effects, and native resources
- terrain and field objects
- active decision owner
- objectives and terminal outcome
- deterministic random-stream state

### Commands

- move
- basic attack
- use ability
- interact with objective
- wait
- end turn
- future commands with typed payloads where an ability requires additional selection

### Events

- movement
- damage, healing, and resource changes
- effect and cooldown changes
- terrain and field-object changes
- death
- active-character changes
- objective changes
- match completion

## Migration strategy

Each gameplay area is migrated as a vertical slice:

1. Add its definition and state data to `RoundCore`.
2. Add legal-action generation, validation, execution, and events.
3. Add standalone unit and deterministic replay tests.
4. Add an Unreal data converter and runtime adapter.
5. Run the old Actor rule and Core rule in comparison-only mode on fixed scenarios.
6. Fix all differences.
7. Switch Unreal execution to Core.
8. Remove the old Actor-side rule implementation.

## Phases

### Phase 0 — Baseline and contracts

- Preserve the pre-migration branch and commit.
- Replace stale architecture guidance.
- Establish standalone build and tests.
- Define IDs, coordinates, commands, events, definitions, state, error reporting, and schema versioning.
- Record baseline Unreal build and playtest results.

Exit condition: `RoundCore` builds without Unreal and contract tests pass.

### Phase 1 — Board, occupancy, and movement

- Export board tiles and terrain movement properties.
- Own character positions and tile occupancy in Core.
- Implement adjacency, path legality, movement cost, stops-movement behavior, and collision rules.
- Convert movement UI intent to a Core command.
- Drive Actor movement from Core movement events.

Exit condition: human movement and standalone movement tests use the same Core rule; no Actor-side movement rule remains.

Current progress:

- Core validates one-field movement against active character, adjacency, occupancy, blocking objects, terrain, movement blocks, and initiative.
- Core derives deterministic movement costs using fixed-point terrain multipliers.
- Legal move actions are generated in stable tile-ID order.
- Successful moves atomically update occupancy and emit movement plus initiative events.
- Unreal now compiles the same implementation file through a dedicated `RoundCore` runtime module; it does not contain a second gameplay implementation.
- A live-board adapter exports sorted tiles, terrain movement rules, blocking objects, resolved character movement costs, health, initiative, teams, occupancy, and the active character into Core state.
- `IsCoreMoveLegal` rebuilds a snapshot on demand, compares Core legality with the Actor result, and logs any mismatch without changing gameplay.
- Normal one-field `MoveToTile` now validates and executes position plus initiative changes in Core. Unreal mirrors the result, runs still-unmigrated enter/movement reactions, records presentation data, and resynchronizes Core afterward.
- Multi-field human movement is decomposed into the same validated Core step action for both host and network clients; the server no longer executes a separate Actor-side movement rule.
- Cached legal-action synchronization and migration of terrain/effect reactions remain before the complete movement slice can remove all Actor-side movement compatibility code.

### Phase 2 — Initiative and decisions

- Move initiative, active-character selection, waiting, end-turn behavior, and delayed-action scheduling.
- Define exactly when a new AI decision is requested.
- Remove animation timing from turn progression.

Exit condition: a standalone match can advance indefinitely through valid move/wait/end-turn commands.

Current progress:

- Core owns deterministic active-character selection, effective initiative thresholds, turn-delay payment, and one-point global initiative advancement.
- Turn advancement deliberately yields after every global initiative point so unmigrated effect, cooldown, terrain, and objective reactions can run in Unreal before Core is resynchronized.
- Actor `EndTurn` reactions still run, but next-turn selection itself is delegated to Core and the selected Actor starts through a compatibility hook that does not charge turn delay twice.
- Ability cooldown and charge regeneration now advance in Core on each global initiative point; Unreal mirrors the resulting runtime values before specialized effect callbacks run.
- Standalone waiting and fully Core-owned effect progression remain to be migrated.

### Phase 3 — Combat backbone

- Basic attacks, range, visibility requirements, damage types, defense, critical hits, death, and victory.
- Emit complete combat events.
- Convert health bars and combat presentation into event consumers.

Exit condition: a standalone minimal match can end with a deterministic winner and Unreal displays the same match.

Current progress:

- Core validates character basic attacks by active actor, enemy team, range, and initiative cost.
- Core resolves fixed-point critical hits, the existing defense curve, health, death, occupancy release, terminal state, and winner events deterministically.
- Unreal exports effective attack range, attack cost, crit chance, and crit multiplier, while preserving the Core random stream across live-state resynchronization.
- Actor effects, terrain/aura damage modifiers, death prevention, board-object attacks, and presentation consumption remain Actor-owned, so live attack execution has not switched yet.

### Phase 4 — Shared effects and terrain

- Cooldowns, durations in initiative, buffs, debuffs, auras, delayed effects, terrain effects, devices, traps, and objectives.
- Establish data-driven effect operations and dedicated Core handlers for genuinely special behavior.

Exit condition: shared systems have parity tests and no migrated effect mutates Actor state directly.

### Phase 5 — Characters and abilities

Migrate one complete character at a time, starting with the simplest supported character. For every ability and passive:

- targeting and legal actions
- costs and cooldowns
- execution and events
- skillpoint values and unique mechanics
- interactions with effects, terrain, and other abilities

Character 6 and Puppet remain last because of their known complexity and bugs.

Exit condition: every playable non-Puppet character is fully Core-driven, followed by Character 6/Puppet stabilization.

### Phase 6 — Vision and observations

- Move line-of-sight and visibility state into Core.
- Produce team-filtered observations with no hidden-information leaks.
- Add stable numeric encodings and legal-action masks.

Exit condition: observation tests prove that hidden enemies and state are not exposed.

### Phase 7 — Data export

- Add an editor commandlet/exporter that converts registries and DataAssets into a versioned binary Core content package.
- Validate missing IDs, invalid references, unsupported mechanics, and schema mismatches.
- Unreal creates the same in-memory definitions directly from editor assets.

Exit condition: Unreal and standalone tests load definitions with identical content hashes.

### Phase 8 — Standalone environment

- Implement `reset`, `legal_actions`, `observe`, `step`, `terminal`, `winner`, `clone`, and state hashing.
- Support many sequential matches without leaks.
- Add a random-valid agent and a heuristic baseline.
- Benchmark steps and matches per second.

Exit condition: random agents complete thousands of deterministic matches without Unreal.

### Phase 9 — Python and training

- Bind the standalone library directly to Python, preferably with `pybind11` outside the Unreal module.
- Provide vectorized environments, action masks, rewards, replay capture, checkpoints, and evaluation tournaments.
- Begin with reduced deterministic matchups, then expand content coverage.

Exit condition: self-play training runs reproducibly and exported models beat random and heuristic baselines.

### Phase 10 — Runtime inference and networking

- Load trained models for optional AI players.
- Keep the server authoritative: clients submit commands, the server steps Core, and replicated events/state update clients.
- Version commands, content, models, and replays.

Exit condition: humans and AI can occupy either team in local and online matches using the same command path.

## Verification gates

Every phase requires:

- standalone unit tests
- deterministic replay test
- invalid-command tests
- Unreal/Core parity scenarios during transition
- performance measurement
- clean standalone and Unreal builds
- documentation of remaining Actor-authoritative rules

## First implementation slice

The first commit establishes:

- the standalone C++17 project
- stable primitive IDs and axial hex coordinates
- definitions, mutable state, commands, results, and events
- reset validation and deterministic state hashing
- contract tests proving independence from Unreal

The following slice will migrate board occupancy and one-field movement end to end.
