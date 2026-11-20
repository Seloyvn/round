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
- Live character attacks now resolve a non-mutating clone of the current Core snapshot before Actor execution and compare initiative cost, applied damage, death, match end, and critical state afterward. Mismatches are logged as `RoundCoreAttackParity` and provide the concrete parity cases needed to migrate shared modifiers without changing current gameplay.
- Critical-hit rolls for live basic attacks now consume the same deterministic RoundCore random stream as headless attacks; Unreal no longer uses its independent global random source for this migrated rule.
- Shared effect state and executable ability damage types are serialized in package version 9. Effect state participates in deterministic hashes, team-filtered observations, Python batches, duration expiry, normal-attack resolution, movement legality and cost, and ability legality. Unreal exports resolved runtime instances including causer restrictions, ranged-only restrictions, multipliers, defense ignore, pre-mitigation lifesteal, one-hit basic-attack damage changes, one-use basic-attack costs, silence, root, the shared slow contract, and Icebeam's movement counters.

### Phase 4 — Shared effects and terrain

- Cooldowns, durations in initiative, buffs, debuffs, auras, delayed effects, terrain effects, devices, traps, and objectives.
- Establish data-driven effect operations and dedicated Core handlers for genuinely special behavior.

Exit condition: shared systems have parity tests and no migrated effect mutates Actor state directly.

Current progress:

- `RoundModifierEffect` is the first explicit shared effect state. Initiative and turn-relative durations expire deterministically and emit removal events.
- Modifier observations redact hidden causer IDs, preventing effects on visible units from leaking unseen enemies to a training policy.
- Additive movement costs and blocking movement-cost bonuses now resolve in the same Core movement rule; Unreal exports the unmodified character base cost so terrain and effects are applied exactly once.
- `RoundBasicAttackDamageReductionEffect` consumption and the first-attack initiative tax from `RoundSuppressingFireEffect` now resolve in Core and are visible to training observations.
- `RoundSilenceEffect` removes ability commands from Core legal-action masks and rejects manually submitted ability commands until its shared effect duration expires.
- `RoundRootEffect` blocks Core movement, applies its causer-specific incoming-damage multiplier, and applies initiative loss when its duration expires. Unreal's initiative callback now only removes the presented Actor instance during Core-driven advancement, avoiding a second initiative loss.
- `RoundSlowEffect` now exports its movement-cost multiplier, movement-bonus block, per-field initiative loss, and expiry loss. Core applies the generic movement penalties, while Unreal movement callbacks only record presentation data and continue running still-unmigrated specialized Icebeam and Suppressing Fire reactions.
- `RoundIcebeamEffect` consumes its optional first-move initiative penalty and expires after its configured number of moved fields in Core. The Actor callback mirrors only runtime presentation state. Frostbreak remains Actor-owned until direct abilities share the complete Core modifier and aura damage pipeline.
- Every exported executable ability now carries its editor-authored damage type through packages and Python. This is the first prerequisite for Frostbreak; the trigger remains Actor-owned until direct abilities use the same complete modifier and aura damage pipeline as normal attacks.
- Duration changes applied by other effects, damage redirection, and specialized effect classes remain Actor-owned until their individual rules are migrated.

### Phase 5 — Characters and abilities

Migrate one complete character at a time, starting with the simplest supported character. For every ability and passive:

- targeting and legal actions
- costs and cooldowns
- execution and events
- skillpoint values and unique mechanics
- interactions with effects, terrain, and other abilities

Character 6 and Puppet remain last because of their known complexity and bugs.

Exit condition: every playable non-Puppet character is fully Core-driven, followed by Character 6/Puppet stabilization.

Current progress:

- The first explicit ability contract supports editor-declared direct damage and direct healing with target relation, range, visibility, initiative cost, cooldown, charges, defense ignore, death, and victory handled by Core.
- Supported abilities appear in stable legal-action lists and numeric action masks; standalone C++ and Python tests execute the same path.
- Unreal exports an ability only into this executable subset when its resolved `CoreOperation` value explicitly opts in. Unmarked and specialized abilities remain unavailable to the headless agent rather than being approximated.
- Unreal execution has not yet switched to this path; parity comparison and selection of the first genuinely simple asset remain before removing its Actor implementation.

### Phase 6 — Vision and observations

- Move line-of-sight and visibility state into Core.
- Produce team-filtered observations with no hidden-information leaks.
- Add stable numeric encodings and legal-action masks.

Exit condition: observation tests prove that hidden enemies and state are not exposed.

Current progress:

- Core computes deterministic per-character and per-team visible tile IDs from character vision range, source-terrain range modifiers, accumulated terrain vision cost, and blocking terrain.
- Blocking terrain remains visible while tiles behind it are hidden, matching the current Unreal rule.
- Unreal exports the live terrain vision values into the same Core definitions.
- Team-filtered structured and numeric observations now remove hidden tiles and enemies, suppress seed/random prediction inputs, and expose legal actions only to the active team.
- Basic attacks require team visibility in Core, preventing legal-action masks from leaking hidden targets.
- Unreal character vision now calls the Core visibility rule through the live-board adapter; the old Actor-side geometric implementation has been removed.
- Vision-changing status effects still apply after geometric team visibility in Unreal and must migrate with the shared effect system.

### Phase 7 — Data export

- Add an editor commandlet/exporter that converts registries and DataAssets into a versioned binary Core content package.
- Validate missing IDs, invalid references, unsupported mechanics, and schema mismatches.
- Unreal creates the same in-memory definitions directly from editor assets.

Exit condition: Unreal and standalone tests load definitions with identical content hashes.

Current progress:

- A versioned, portable binary match package serializes resolved Core definitions and complete resumable state with deterministic ordering and bounded input validation.
- Standalone C++ and Python load the same package and reject invalid magic, unsupported versions, truncated data, and trailing bytes.
- A running Unreal match can export its live resolved Core snapshot with `ExportCoreTrainingPackage`; an empty path writes `Saved/Training/roundcore_match.rndcore`.
- Registry-wide content export before starting a match and a stable content hash remain.
- Resolved additive ability values and each character's exact ability-definition list are now included in live Unreal exports; runtime packages reject missing, duplicate, undeclared, or invalid ability state before training starts.

### Phase 8 — Standalone environment

- Implement `reset`, `legal_actions`, `observe`, `step`, `terminal`, `winner`, `clone`, and state hashing.
- Support many sequential matches without leaks.
- Add a random-valid agent and a heuristic baseline.
- Benchmark steps and matches per second.

Exit condition: random agents complete thousands of deterministic matches without Unreal.

Current progress:

- `ApplyDecision` advances an end-turn command through one-point initiative ticks until the next decision owner is available, while the lower-level one-tick API remains available to Unreal during migration.
- A standalone self-play smoke executable uses only Core legal actions and commands to complete 500 rendered-free matches per test run.
- `ObserveDecision` snapshots state together with its legal action set, and `ApplyDecisionIndex` gives trainers an index-based step interface without bypassing Core validation.
- Core matches can be cloned for independent rollouts without shared mutable state.
- A deterministic random-valid baseline completes thousands of standalone matches and reports throughput in the test output.
- A versioned observation schema maps editor-exported definition IDs to deterministic numeric indices, and omniscient state/action tensors are emitted in stable entity order.
- `VectorEnvironment` resets, observes, and steps independent Core matches in batches while preserving per-environment results and legal actions.
- The optional `roundcore_python` module exposes Core definitions, setup, observations, indexed decisions, and vector environments without linking Python into Unreal.
- `roundcore_training.encode_batch` converts variable-length observations into fixed NumPy tensors and explicit masks using caller-configured capacities.
- Configurable Python reward calculation consumes Core events without becoming a second gameplay implementation, and replay traces verify every indexed decision against before/after state hashes.
- Both explicit omniscient observations and visibility-filtered team observations are available; training policies should use the team-filtered API.
- `SelfPlayEnvironment` loads an Unreal-exported package, advances to a decision boundary, returns only the acting team's filtered numeric observation, accepts a legal-action index, calculates event-based reward, and records deterministic replay hashes.
- A package smoke runner can execute a real exported setup without rendering and reports when an unmigrated mechanic prevents the match from completing.
- The first real 5v5 Unreal export (721 tiles, 50 abilities, 19 terrain definitions) loads successfully and completes headlessly in 420 decisions with the deterministic tactical baseline.
- Live basic attacks use RoundCore's deterministic critical stream, and resolved incoming-damage auras now enter the generic Core passive contract with their range, reduction, self-target rule, and prevention rewards.
- Visibility requests made while a character is still being spawned are treated as an incomplete lifecycle state rather than a Core failure; registered characters continue to use only the Core visibility rule.
- Content-package loading, larger match coverage, and formal performance benchmarks remain.

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
