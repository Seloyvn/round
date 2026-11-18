# Round Project Notes

This file captures project-specific conventions and decisions for working in this repository.

## Core Architecture

- The standalone, Unreal-independent `RoundCore` is the target authority for gameplay state and rules.
- Do not create a separate approximate training simulation. The training executable and Unreal must call the same `RoundCore` implementation.
- Migrate rules in vertical slices. Until a rule is migrated, its Actor implementation may remain authoritative. After migration, Actors submit commands and present Core events instead of executing that rule again.
- Unreal owns presentation, input, networking adapters, asset authoring, and Actor lifetime. `RoundCore` owns migrated gameplay state, validation, deterministic resolution, and results.
- Balance and design values remain editor-authored and are exported into versioned Core definitions; they must not be hardcoded into the standalone rules.

## Data and Registries

- Use data asset references instead of hardcoded file paths whenever possible.
- The root reference point for shared game data is `DA_RoundGameDataRegistry`.
- Character data should be resolved through the character registry, not by fixed `/Game/...` assumptions.
- Ability data should be resolved through ability assets and registries, not duplicated IDs in multiple places.

## Editor-Owned Design and Balance Data

- Values that may reasonably change during balancing, content design, map design, progression design, or playtesting must never be hardcoded in runtime code.
- Store tunable values in editor-managed DataAssets, DataTables, registries, format definitions, or other appropriate Unreal assets.
- Runtime code should implement behavior, validation, and interpretation of data; it should not define the editable balance values themselves.
- This applies across the project, including character stats, ability values, skillpoints, terrain behavior, terrain generation, region sizes, region frequency, adjacency compatibility, transition rules, spawn protection, objectives, team formats, and presentation tuning.
- Defaults in C++ are only defensive fallbacks for missing or invalid data. They must not silently become the primary source for designed content.
- Prefer stable data identifiers and generic collections over one C++ property or branch per content type. Adding a new character, ability, terrain field, terrain region, formation, or adjacency rule should normally be possible in the Unreal Editor without adding another hardcoded value column or switch branch.
- Shared configurable data should be reachable through the relevant registry and ultimately through `DA_RoundGameDataRegistry`, rather than discovered through fixed asset paths.
- Validate editor-authored data at load or generation time and report missing entries, asymmetric matrices, invalid ranges, and impossible combinations clearly in logs.

## Characters

- New characters should get:
  - a runtime character class
  - a character data asset
  - a custom UI widget class if they expose unique state
  - ability data assets and ability actor classes as needed
- Character-specific passive logic should live in the character class or dedicated effect classes, not in generic catch-all systems.
- Puppet-only special cases may be exempt from standard ability expectations.

## Character Models

- The current `Character_1` Blender model is considered a good first real placeholder baseline.
- For early 3D character production, prefer readable midpoly placeholders over very rough blockouts once the gameplay silhouette is stable.
- Character silhouettes should read clearly from the in-game isometric perspective first.
- Weapon identity should come through the full body silhouette and animated props, not only from small hand-held details.
- For the chain tank specifically, the character should fight through chain-whip shapes rather than conventional direct weapons.

## Abilities

- Ability execution is actor-based.
- Prefer ability-specific child classes when behavior is distinct.
- Generic base ability logic is acceptable for simple single-target or shared utility behavior.
- More specialized abilities should override execution directly instead of overloading the generic path with too many conditionals.

## Ability Values

- Ability key-values are additive.
- Do not use a max-of-values model for skillpoint scaling.
- Example:
  - base `AOERange = 1`
  - skillpoint adds `AOERange = 1`
  - resolved value becomes `2`
- `DamageAdd` is not needed under this model; use the actual value key instead.

## Skillpoint Design

These are design guidelines, not inflexible rules. A well-justified point may break an individual guideline when that produces a better fit for the character, ability, and actual game flow.

### Required analysis before designing points

- Before proposing any character or ability skillpoints, first analyze the existing character or base ability in its current implemented form.
- Summarize the character's role and theme, or the ability's purpose and practical use during a match.
- Identify relevant base values, targeting, costs, cooldown, range, damage, resources, movement, positioning, effects, and natural follow-up actions.
- Note existing strengths, limitations, risks, and interactions with the rest of the kit.
- State what the progression tree should enable, which build directions fit, and which directions would conflict with the character or ability.
- Only design the individual points after this analysis has established a clear purpose for the tree.

### Tree structure and unlocking

- Character and ability progression trees use four rows with three choices per row.
- A player may select exactly one point in each row.
- Selecting a point in row 1 is required to unlock row 2; each later row likewise requires a selection in the preceding row.
- Points in the same row must work independently because they cannot be selected together.
- Options in the same row should be meaningful alternatives, not stronger and weaker versions of effectively the same result.

### Character and ability identity

- The primary requirement is that a point fits the character's role, theme, and intended playstyle as well as the base ability's real gameplay flow.
- Start from what the base ability actually targets, costs, produces, consumes, and enables afterward.
- Do not attach a mechanic merely because it is generically powerful or thematic.
- Rewards should fit the role. For example, personal kill rewards are usually inappropriate for a dedicated supporter, while protection, enabling allies, and recovery are natural support rewards.
- Conditions should represent actions the character naturally wants to take and that players can deliberately create.

### Rows and power progression

- Rows are not separated into themes such as stats, debuffs, combo, or utility. Mix different kinds of options within a row.
- Earlier rows generally favor clear, reliable, and less complex effects.
- Later rows may contain stronger, more conditional, or more mechanically involved effects.
- Row 4 does not have to be complex or transform the ability. A strong and appropriate simple improvement is valid.
- Complexity is not valuable by itself and should only be used when it creates a useful decision or build direction.

### Numeric and unique effects

- Straightforward improvements such as damage, cooldown, range, cost, duration, or defense ignore are valid skillpoints.
- Do not let every option become a numeric increase, especially when that creates a mathematically dominant choice.
- Unique effects should create practical decisions, synergies, positioning options, resource patterns, or team interactions.
- A unique effect must function in normal gameplay and be understandable; complicated but impractical effects are not desirable.
- Conditional options may be stronger than reliable options when their condition requires meaningful setup.

### Tradeoffs

- Downsides are allowed when there is a good design reason.
- Use a downside to balance an unusually strong benefit, define a specialization, or create meaningful risk and reward.
- Do not add a downside only to make a point appear balanced or more complicated.
- The benefit should clearly explain why accepting the downside is worthwhile.

### Interactions with other abilities

- A skillpoint may interact with another ability but should not rewrite or directly modify that other ability's definition.
- Valid interactions include creating or consuming shared resources, preparing a target, responding to a state, or reducing another ability's current cooldown.
- Avoid changing another ability's base damage, targeting rules, or permanent values from an unrelated skill tree.

### Runtime and wording checks

- Design against the real actor-based gameplay flow, not an imagined sequence that the game does not support.
- Check movement triggers, multi-hit effects, refunds, cooldown resets, and reactions for loops or unintended repeated triggers.
- One-field movement must not enable infinite initiative or turn loops.
- Radius-dependent points must still make sense with the ability's available radius.
- Debuff durations normally use initiative. Use turn-relative duration only when the turn boundary itself is important.
- State explicitly whether an effect is once per turn, unlimited, stackable, consumed on use, or restricted to a particular source.
- Descriptions should identify the trigger, target, magnitude, duration, consumption behavior, and trigger limit whenever those details apply.

### Review questions

Before accepting a point, check:

1. Does it fit the character's role and the ability's purpose?
2. Does it work in the actual gameplay sequence?
3. Is it a real alternative to the other two points in its row?
4. Is another point already doing effectively the same thing?
5. Is its condition understandable, reachable, and player-controlled?
6. Can it create loops or unintended repeated triggers?
7. Are trigger, strength, duration, stacking, consumption, and limits described clearly?
8. If it has a downside, is that downside justified by the benefit?
9. Is its complexity appropriate and useful?

## Delayed Abilities

- Delayed ability execution uses `DelayedExecutionInitiative`.
- Alias `DelayAbilityInitiative` is also supported.
- By default:
  - the ability cost is paid immediately
  - cooldown is set immediately
  - the current turn ends immediately
  - the caster cannot take turns while the delayed ability is pending
  - the delayed ability itself is locked while pending
- Override with:
  - `AllowTurnsWhileDelayed = 1`
  - or `CanActWhileDelayed = 1`

## Effects

- `ARoundEffectActor` is the generic runtime base.
- Effects may classify themselves with:
  - `IsBuff()`
  - `IsDebuff()`
  - `IsRemovable()`
- Default:
  - removable = true
- Auras are not removable by default.
- If an effect is specialized, prefer a dedicated child class over packing more behavior into a generic modifier.

## Auras

- `ARoundAuraEffect` should only handle generic aura/range behavior.
- Actual aura gameplay logic belongs in child classes.
- Parent aura should not contain specialized modifier logic.
- Examples:
  - Paladin aura damage reduction in a dedicated child
  - Sanctified Ground damage in a dedicated child

## Support / Cleric

- The support character theme is `Cleric`.
- The Cleric passive uses `Blessing`.
- Allies affected by Cleric spells should receive or refresh Blessing.
- Blessing stacks when:
  - the Cleric affects the ally with a spell
  - the blessed ally takes damage
- Blessing resets duration on stack gain.
- At max stacks, Blessing heals the ally and resets.

### Blessing keys

- `BlessingDurationInitiative`
- `BlessingMaxStacks`
- `BlessingHeal`

## UI

- Shared UI classes should be referenced from `DA_RoundGameDataRegistry`.
- Character-specific custom UI widgets should live in `RoundChampionCustomUIWidget` child classes.
- If a character has unique runtime state, expose it through a dedicated custom UI widget.

## Naming

- Prefer clear, role-based names for new characters and abilities.
- Supporter/protector character is `Cleric`.
- Keep ability IDs stable and data-driven.

## Build Notes

- Unreal builds may fail from memory pressure when too many compile actions run in parallel.
- If PCH/virtual memory errors appear, rerun with fewer parallel actions, for example:
  - `-MaxParallelActions=2`

## General Refactor Guidance

- Prefer small, explicit child classes over large generic branches.
- Remove stale comments that refer to old sim behavior.
- Keep new gameplay behavior traceable from:
  - character
  - ability
  - effect
  - UI
