# Simulation And AI Plan

## Goal

Build a high-performance AI training stack for Round without duplicating gameplay logic between Python and C++ and without breaking the existing Unreal game.

## Current Implementation Status

The first non-invasive sim foundation is now in the project.

Implemented files:

- `Source/Round/Public/Sim/RoundSimTypes.h`
- `Source/Round/Public/Sim/RoundSimBuilder.h`
- `Source/Round/Private/Sim/RoundSimBuilder.cpp`
- `Source/Round/Public/Sim/RoundSimRules.h`
- `Source/Round/Private/Sim/RoundSimRules.cpp`
- `Source/Round/Public/Sim/RoundSimFacade.h`
- `Source/Round/Private/Sim/RoundSimFacade.cpp`
- `Source/Round/Public/Sim/RoundSimEnvironment.h`
- `Source/Round/Private/Sim/RoundSimEnvironment.cpp`
- `Source/Round/Public/Sim/RoundSimApi.h`
- `Source/Round/Private/Sim/RoundSimApi.cpp`
- `Source/Round/Public/Sim/RoundSimPythonTypes.h`
- `Source/Round/Private/Sim/RoundSimPythonTypes.cpp`
- `Source/Round/Public/Sim/RoundSimPythonEnvironment.h`
- `Source/Round/Private/Sim/RoundSimPythonEnvironment.cpp`
- `Source/Round/Public/Sim/RoundSimEncoding.h`
- `Source/Round/Private/Sim/RoundSimEncoding.cpp`
- `Source/Round/Public/Sim/RoundSimRuntimeBridge.h`
- `Source/Round/Private/Sim/RoundSimRuntimeBridge.cpp`

What this foundation already provides:

- actor-free simulation data structs
- a full live match snapshot builder from `ARoundGameState`
- a team-based observation builder with optional fog-of-war filtering
- an initial executable rule API for:
  - turn advancement
  - legal action generation
  - move resolution
  - basic attack resolution
  - end turn
- a first shared effect pipeline in the sim core for:
  - cooldown ticking
  - bleed
  - slow move penalties
  - generic modifier effects
  - invisibility visibility filtering
  - taunt-like outgoing penalties
  - concentrate-style one-hit buffs
- a first generic ability execution path based on ability metadata
- a runtime bridge that can:
  - build sim actions from live character intent
  - query legal actions for the live game
  - validate runtime move / attack / ability intents against the sim core
- a sim facade layer that can:
  - build decision contexts from arbitrary sim states
  - build live decision contexts directly from `ARoundGameState`
  - step a sim state and return the resulting state, observation, and legal actions together
- an environment layer that can:
  - hold an initial and current sim state
  - `reset()` back to the initial state
  - return the current decision context on demand
  - step by raw action, indexed action, or action key
  - initialize from arbitrary sim state or directly from a live `ARoundGameState`
- an API/export layer that can:
  - convert indexed actions into stable action info records
  - expose compact environment state views
  - expose step results in a binding-friendly container
  - step environments by raw action, action index, or action key
- a binding-friendly Python-facing wrapper layer that can:
  - convert Unreal/Blueprint sim views into plain C++ string/vector based records
  - hold sim state without `UObject` ownership
  - expose reset/state/step semantics in a form that `pybind11` can bind directly
- stable action encoding helpers:
  - indexed legal actions
  - canonical action keys
  - readable action labels
- outcome and reward helpers:
  - perspective-based state summaries
  - terminal / win / loss detection
  - health / alive / initiative deltas per step
  - first shaped reward breakdown for AI-facing `step()` results
- no change to the visible player-facing game flow

What is intentionally not migrated yet:

- full per-ability gameplay parity
- effect ticking inside the sim core
- full parity for passive, effect, terrain-special, crit, and class-native rules
- runtime gameplay still executing through the original actor system instead of the sim core

## First Runtime Connection

The live runtime now exposes sim-side access through `ARoundGameState`:

- `BuildCurrentSimState()`
- `BuildCurrentSimObservation(...)`
- `BuildCurrentSimDecisionContext(...)`
- `GetCurrentLegalSimActions()`
- `GetCurrentIndexedLegalSimActions(...)`
- `BuildCurrentSimOutcomeSummary(...)`
- `StepCurrentSimAction(...)`
- `IsSimMoveLegal(...)`
- `IsSimAttackLegal(...)`
- `IsSimAbilityLegal(...)`

The current runtime uses the sim bridge conservatively:

- move legality checks now consult the sim bridge
- basic attack legality now consults the sim bridge
- ability target validation now consults the sim bridge
- direct ability use now also checks sim legality before execution
- move, attack, and ability execution now also run through sim-side shadow prediction for parity logging

The `ARoundGameState` sim entry points now also route more work through the shared sim facade instead of directly stitching together builder and rules calls. This is an important migration step because it makes the runtime consume the same higher-level sim API that a future Python binding will use.

This is an intentional halfway step:

- the visible gameplay still resolves through the original actor code
- but the live game is beginning to share decision validation with the sim core

That means the project now has:

- a stable bridge from the current Unreal runtime into pure sim data
- a first executable, deterministic sim loop for the simplest core actions

This is the correct starting point for the larger migration.

## New AI-Facing Layer

The sim facade now exposes not only state, observation, and legal actions, but also:

- `FRoundSimOutcomeSummary`
- `FRoundSimRewardBreakdown`

These are computed from pure sim state and attached to `FRoundSimStepBundle`.

That means the sim API now covers the first full AI loop shape:

- get current decision context
- choose one indexed legal action
- step the state
- receive:
  - next state
  - next observation
  - next legal actions
  - terminal flag
  - outcome summary
  - shaped reward breakdown

This is still not the final Python binding, but it means the sim core now already produces the right data model for one-step training loops without inventing reward logic on the Python side.

The target architecture is:

- one gameplay ruleset in C++
- one fast simulation core in C++
- Unreal uses that same ruleset for the playable game
- Python uses bindings into that same simulation core for training and analysis

This avoids:

- rule drift between Python and C++
- gameplay bugs existing only in one implementation
- slow JSON or file-based training loops

## Non-Negotiable Requirements

- The current Unreal game must continue to work for human players.
- Gameplay logic must exist only once.
- Training must be fast enough for large amounts of self-play or repeated simulation.
- The system must be deterministic enough for debugging, replay inspection, and reproducible AI runs.
- The architecture should support future content without redoing the whole AI integration.

## High-Level Architecture

### Final target

1. `Sim Core` in C++
   - no `AActor`
   - no `UUserWidget`
   - no world or rendering dependency
   - pure state + rules + action resolution

2. `Unreal Runtime Adapter`
   - translates between live Unreal actors and the simulation state
   - lets the current game keep using graphics, UI, VFX, sound, and interaction

3. `Python Binding Layer`
   - direct binding to the C++ sim core
   - no JSON in the hot training loop
   - Python receives observations, legal actions, rewards, terminal flags

4. `AI Layer`
   - Python for training, experimentation, logging, evaluation
   - optional C++ bots for heuristic or search-based AI

### What should not be part of the sim core

- Actor spawning
- replication
- widgets
- hover states
- visual effects
- animation timing
- player controller logic
- UI-only concepts

## Why Not Rebuild The Game In Python

That would create two rule implementations:

- Python for AI
- C++/Unreal for the actual game

That is the exact scenario we want to avoid.

## Why Not Use JSON Communication For Training

JSON or socket communication is acceptable for tooling or debugging, but not for the main training path.

JSON-based communication would mean:

- serializing state every step
- parsing every action and response
- higher overhead in massive simulation workloads

For heavy AI training, the right long-term solution is:

- direct Python bindings into the C++ sim core

## Recommended Technical Direction

### Best overall setup

- Simulation rules: C++
- Training orchestration: Python
- Communication: direct binding, ideally `pybind11`

This gives:

- Python ML ecosystem
- C++ simulation speed
- one source of truth for gameplay rules

## Refactor Strategy

This should be done inside the existing project, not as a separate gameplay codebase.

Recommended location:

- `Source/Round/Public/Sim/...`
- `Source/Round/Private/Sim/...`

The Unreal actor game remains in the same project and gradually moves to use the sim core.

## Current Reality

Right now, gameplay logic is heavily embedded in:

- `ARoundCharacter`
- `ARoundAbilityActor`
- `ARoundGameState`
- effect actors
- terrain and board actors
- UI-driven assumptions

That means the refactor is significant.

However, much of the rule content can be preserved:

- damage formulas
- cooldown formulas
- initiative logic
- ability values
- terrain effects
- effect durations
- team and victory rules

The main change is not "new rules".
The main change is "new ownership and structure of rules".

## Refactor Phases

## Phase 0: Safety And Baseline

### Goal

Create a stable starting point before moving logic.

### Tasks

- keep a full backup
- create a dedicated refactor branch
- document current architecture
- list known gameplay systems
- identify deterministic and non-deterministic behavior
- keep current match JSON logging working

### Deliverables

- branch / backup complete
- architecture notes
- baseline playtest checklist

## Phase 1: Define The Sim Data Model

### Goal

Create the pure simulation state structures without implementing the full rules yet.

### Core types to introduce

- `FRoundSimState`
- `FRoundSimBoardState`
- `FRoundSimTileState`
- `FRoundSimCharacterState`
- `FRoundSimAbilityState`
- `FRoundSimEffectState`
- `FRoundSimTeamState`
- `FRoundSimAction`
- `FRoundSimActionResult`
- `FRoundSimObservation`

### Important properties

For each character:

- id
- team
- alive / dead
- current tile
- health
- initiative
- base stats
- passive selections
- learned abilities
- ability cooldowns
- active effects
- class-specific native state

For each tile:

- position
- terrain type
- occupant id
- temporary terrain state if relevant

For match state:

- active character
- turn order information
- winner / terminal state
- seed or deterministic context if needed later

### Deliverables

- compile-safe sim state types
- no actor or widget dependency
- initial live-state snapshot builder
- initial observation builder for one viewer team

## Phase 2: Build The Rule API Surface

### Goal

Define the exact API that both Unreal and Python will depend on.

### Core API

- `ResetMatch(...)`
- `BuildStateFromTeamDefinitions(...)`
- `GetLegalActions(const FRoundSimState&, PlayerView)`
- `ApplyAction(FRoundSimState&, const FRoundSimAction&)`
- `AdvanceUntilNextDecision(FRoundSimState&)`
- `IsTerminal(const FRoundSimState&)`
- `BuildObservation(const FRoundSimState&, PlayerView)`
- `CloneState(...)` or efficient copy/undo model

### Action categories

- move
- attack
- cast ability
- end turn
- wait if still needed by rules

### Deliverables

- stable simulation API shape
- explicit action encoding plan

## Phase 3: Move Core Systems First

### Goal

Move the systems with the highest architectural value before touching every ability.

### Order

1. board and occupancy
2. movement rules and path legality
3. initiative and turn selection
4. basic attack
5. damage and defense
6. death and victory

### Why this order

These systems are the backbone for everything else.
Once they are stable in the sim core, later ability migration becomes much safer.

### Deliverables

- headless movement and attack simulation
- turn progression in pure C++

## Phase 4: Effects And Shared Gameplay Rules

### Goal

Move reusable systems before character-specific content.

### Systems to extract

- cooldown handling
- status effects
- debuff / buff application
- terrain interactions
- vision rules if needed for AI observation
- area / aura logic

### Deliverables

- generic effect resolution in sim core
- generic cooldown and duration model

## Phase 5: Ability Migration

### Goal

Move all ability resolution out of actor-heavy logic into the sim core.

### Recommended order

- Character 1
- Character 2
- Character 3
- Character 4
- Character 5
- Character 6

### Why this order

Complexity grows toward Character 6.
We should validate the pattern on simpler characters first.

### Deliverables

- each ability represented as pure simulation logic
- no gameplay-only dependencies on live actors for ability resolution

## Phase 6: Character Passives And Class-Specific Native State

### Goal

Move each character's custom passive machinery into the sim core.

### Examples

- Character 4 element bonus tracking
- Character 5 blood system
- Character 6 puppet coordination state
- Character 3 combo and invisibility

### Deliverables

- character-specific systems fully represented in `FRoundSimCharacterState`

## Phase 7: Unreal Runtime Adapter

### Goal

Keep the current game playable while transitioning it to the sim core.

### Responsibilities

- initialize sim state from match setup
- convert player actions to sim actions
- step sim state
- apply sim results to visible actors and UI
- continue using visuals and widgets from current Unreal code

### Important principle

The actor layer should become a view/controller layer, not the ultimate gameplay truth.

### Deliverables

- playable Unreal match backed by sim state

## Phase 8: Verification And Differential Testing

### Goal

Make sure the sim and the current game behavior match during transition.

### Methods

- replay the same situations in old and new paths
- compare legal action sets
- compare damage outputs
- compare cooldowns
- compare effect durations
- compare winners from fixed scripted scenarios

### Best tools

- scripted scenario tests
- JSON match logs
- deterministic test seeds

### Deliverables

- regression suite for gameplay rules

## Phase 9: Python Binding Layer

### Goal

Expose the sim core to Python without duplicating rules.

### Preferred solution

- `pybind11`

### Exposed API

- `reset()`
- `load_match(team_a, team_b, seed)`
- `get_observation(player_id)`
- `get_legal_actions(player_id)`
- `step(action)`
- `is_terminal()`
- `winner()`
- `clone()` if needed for search

### Notes

- avoid JSON in the training loop
- keep state transfer compact
- observations should be numerical or easily tensorized

### Deliverables

- importable Python module backed by C++

## Phase 10: First AI Layer

### Goal

Get useful AI behavior before going deep into expensive learning.

### Recommended order

1. random valid bot
2. heuristic bot
3. search bot
4. learning bot

### Why

This gives:

- validation that the sim API is usable
- immediate baselines
- stronger debugging tools
- a better starting point for RL later

### Deliverables

- baseline agents
- benchmark numbers

## Phase 11: Learning AI

### Goal

Train a model that improves over time through repeated simulation.

### Likely approaches

- supervised imitation from scripted or search-generated play
- reinforcement learning with self-play
- hybrid approach:
  - heuristic or search teacher
  - learned policy student

### Important requirement

Before large-scale RL:

- sim must be deterministic
- legal action encoding must be stable
- observation encoding must be efficient
- rollout throughput must be measured

### Deliverables

- trainable Python environment over the C++ sim core

## Performance Priorities

These matter more than almost anything else:

1. compact state layout
2. no actor dependency in the hot path
3. cheap action application
4. efficient legal action generation
5. minimal allocation during stepping
6. minimal data copying between C++ and Python

## Recommended State Handling Strategy

Choose one early:

- full state copy per step
- state diff + undo

For simple correctness, full copies are easier.
For heavy search, undo systems may become better.

A practical approach:

- start with clean copy-based logic
- optimize later if profiling shows it is needed

## Observation Design

The AI should not receive raw Unreal actor references.

It should receive a structured player-view observation:

- visible board state
- visible enemies
- allied states
- current actor
- legal actions
- cooldowns
- effects
- terrain

Potential versions:

- raw structured observation for debugging
- compact tensor-ready observation for training

## Legal Action Design

This is critical.

Each legal action should have:

- stable action type
- stable parameters
- deterministic ordering
- easy conversion to and from Python

Example categories:

- `Move(tile_id)`
- `Attack(target_id)`
- `Ability(ability_index, target_tile_id or target_id)`
- `EndTurn`

## Replay And Logging

The current JSON match logging is useful and should stay.

Future improvements:

- cleaner event payloads
- explicit source / target character rows
- clear death and kill records
- deterministic action logs

This is useful for:

- debugging
- AI dataset generation
- match analysis
- replay tooling

## Risks

### Biggest risks

- trying to migrate everything at once
- keeping actor logic and sim logic both active for too long
- hidden dependencies on Unreal world state
- too much serialization overhead in the training path
- not validating behavior during migration

### Mitigation

- migrate in phases
- differential test early
- measure performance regularly
- keep the sim core pure

## Recommended Milestones

### Milestone 1

Pure sim state can:

- load teams
- place units
- move
- attack
- advance turns
- end match

### Milestone 2

Character 1 to 3 fully supported in sim.

### Milestone 3

Character 4 to 6 fully supported in sim.

### Milestone 4

Playable Unreal match uses the sim core for real rules.

### Milestone 5

Python can run self-play against the sim core directly.

### Milestone 6

Baseline heuristic or search AI is working.

### Milestone 7

Learning loop is running with acceptable throughput.

## What To Do First

When implementation starts, the first concrete tasks should be:

1. create `Sim` folders and base structs
2. define `FRoundSimState` and `FRoundSimAction`
3. move board occupancy and turn progression into sim code
4. create deterministic tests for move, attack, cooldown, death
5. keep Unreal gameplay working while sim tests grow

## Final Recommendation

The best long-term path is:

- one ruleset in C++
- no duplicate Python gameplay rules
- Unreal as presentation and live play layer
- Python as AI training layer
- direct binding between Python and the C++ sim core

This is the highest-efficiency architecture for both correctness and training speed, even if it requires a larger upfront refactor.
