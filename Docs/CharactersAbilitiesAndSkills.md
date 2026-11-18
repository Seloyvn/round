# Round Characters, Abilities and Skills

## Scope

This file documents the current gameplay roster:

- character identities
- core passives and resources
- character progression themes
- all current abilities, including the fifth skill added to every character
- the intended role of each ability skill tree

The exact imported numbers live in the data assets and progression assets. Visible skillpoint names and descriptions also come from the progression data assets themselves, not from fixed UI text in C++.

For example, Character 1's character skill tree is authored in:

- `Content/TopDown/Characters/Character_1/DA_Character1Progression.uasset`

This file is the design and implementation overview for the current state of the project.

## Team Presets

When no team save exists yet, the game loads its default presets from
`DA_DefaultTeamPresets`. The asset is referenced by `DA_RoundGameDataRegistry`,
so team names, members, positions and point allocations can be edited in Unreal
without changing C++.

Current preset teams:

- `Frontdruck`
- `Jagdkommando`
- `Raumkontrolle`
- `Blutjagd`

These presets now include:

- exactly 5 selected characters
- start positions
- exactly 4 learned abilities per character
- exactly 16 spent points per character
- point allocations that are valid for the format and still show each team's intended strengths

## Character 1

### Role

Character 1 is a frontline control tank. He holds enemies in place, breaks their tempo, peels for allies and prepares targets for follow-up.

### Abilities

- `Knockback`
  Single target melee hit with displacement.
- `Delay`
  Damage plus initiative control.
- `Shackle`
  Slow and movement control.
- `Bind`
  Trap-like movement punishment.
- `Taunt`
  Threat control and target redirection.

### Character progression

Character 1's character tree is built around:

- frontline durability
- chain economy and recovery
- better sticking power
- forcing enemies to respect his zone

The current concrete Tier names, descriptions and displayed values are read from the progression asset, currently `DA_Character1Progression`.

At a gameplay level, the current Character 1 character tree covers:

- extra chain capacity
- initiative refund on the first chain spend each turn
- reward for ending a turn without moving
- faster chain recharge
- protection against enemies already debuffed by Character 1
- emergency recovery when starting a turn at 0 chains
- durability while chains are available
- overflow conversion from extra recharge into initiative
- longer debuff duration
- cheat death through chain consumption
- partial resistance to delay and enemy forced movement
- bonus defense while at 0 chains

### Ability progression themes

`Knockback`

- Base role: peel, displacement, setup
- Current redesign direction:
  - Row 1
    - more damage
    - initiative loss after push
    - bonus damage if stopped by a wall
  - Row 2
    - more push distance
    - heal Character 1 for each debuff already on the target
    - lower or remove chain cost
  - Row 3
    - collision with a second enemy damages that enemy based on the pushed target's max health
    - pushing the target the full distance extends existing debuffs on it
    - consume all debuffs on the target to deal bonus damage based on combined remaining duration
  - Row 4
    - reactivate once this turn to dash to the target and deal damage
    - also hit fields next to the target field
    - apply a weakened Shackle on the target tile with reduced damage and duration

`Delay`

- Base role: turn control
- Tree themes:
  - deeper delay and attack suppression
  - extending later debuffs
  - small area tempo disruption

`Shackle`

- Base role: movement denial
- Tree themes:
  - stronger or longer slow
  - remove movement buffs
  - punish attempts to move anyway

`Bind`

- Base role: punish movement and greed
- Tree themes:
  - stronger trigger damage or trigger delay
  - anti-mobility lock
  - secondary control when the bind is triggered

`Taunt`

- Base role: frontline protection and target forcing
- Tree themes:
  - stronger taunt pressure
  - defensive punishment if the target ignores Character 1
  - target exposure and anti-range utility

## Character 2

### Role

Character 2 is a patient ranged marksman. He plays around positioning, tempo windows, ammo and precise punishment.

### Abilities

- `Snipe`
  Long-range single target shot that scales with missing initiative on the target.
- `Concentrate`
  Self-buff that prepares a stronger next attack.
- `Aim`
  Trap-like ranged field trigger.
- `PiercingShot`
  Line attack through multiple targets.
- `SuppressingFire`
  Midrange pressure shot that limits movement.

### Character progression

Character 2's character tree supports:

- better ranged pressure
- ammo management
- anti-range tools
- stationary sniper play
- low-ammo and full-ammo timing windows

### Ability progression themes

`Snipe`

- Base role: precise pick tool
- Tree themes:
  - more base damage or better scaling
  - punish isolated targets
  - stronger execution against low or delayed targets

`Concentrate`

- Base role: self setup
- Tree themes:
  - stronger stored shot
  - defense ignore or initiative drain on the next hit
  - safe setup and delayed payoff

`Aim`

- Base role: space control and movement punishment
- Tree themes:
  - larger or deadlier trigger zone
  - anti-mobility effects on trigger
  - better follow-up for Character 2 himself

`PiercingShot`

- Base role: line damage and multi-target pressure
- Tree themes:
  - stronger line hits
  - armor breaking and initiative disruption
  - reward for good lines or stationary firing

`SuppressingFire`

- Base role: pinning targets in place
- Tree themes:
  - stronger movement tax
  - anti-mobility and anti-escape pressure
  - stronger value against already moving targets

## Character 3

### Role

Character 3 is a mobile assassin with combo and invisibility tools. He can either set up a clean kill or pressure wounded targets and collapse quickly.

### Abilities

- `Shadowstrike`
  Mobile strike with invisibility utility.
- `Armorpen`
  Combo builder and armor breaking pressure.
- `Execute`
  Finisher that converts combo into burst.
- `HookShot`
  Pull / engage / reposition tool.
- `SmokeStep`
  Reposition and stealth-oriented utility movement.

### Character progression

Character 3's character tree supports:

- combo economy
- burst vs control assassin directions
- invisibility windows
- bleed pressure
- tempo on surviving targets

### Ability progression themes

`Shadowstrike`

- Base role: engage and vanish
- Tree themes:
  - stronger first contact
  - safety after engagement
  - better damage against already softened targets

`Armorpen`

- Base role: combo generation and target opening
- Tree themes:
  - stronger armor break and chip
  - better pressure on debuffed or low-health targets
  - bleed conversion and anti-attack follow-up

`Execute`

- Base role: kill confirm
- Tree themes:
  - more combo payoff
  - better performance into low-health targets
  - reward on kill or partial refund on big combo use

`HookShot`

- Base role: engage, pull and prep
- Tree themes:
  - extra setup for later hits
  - anti-mobility and spacing control
  - combo support and multi-target disturbance

`SmokeStep`

- Base role: reposition, stealth and setup
- Tree themes:
  - safer retreat or better angle taking
  - invisibility extension
  - stronger next hit after repositioning

## Character 4

### Role

Character 4 is the flexible ranged mage. He is built around adapting to the situation with damage, utility, tempo and terrain control.

### Passive

Character 4 gains an element bonus based on the damage type of the spell he used:

- `Fire`
  stronger next damage event
- `Ice`
  longer next applied effect
- `Lightning`
  initiative advantage on the next spell
- `Rock`
  reduced next incoming damage

Multiple bonuses can be active at the same time. The UI shows whether an element is ready, active or already used this turn.

### Abilities

- `Fireball`
  AoE damage spell.
- `Icebeam`
  damage plus control.
- `Lightningstrike`
  line-based tempo spell.
- `Rockblast`
  damage plus terrain and space control.
- `ArcaneShift`
  repositioning spell.

### Character progression

Character 4's character tree supports:

- efficient casting and turn rhythm
- movement and safety
- bonus value from changing elements
- stronger use of multiple active element buffs
- keeping bonuses longer and converting them into tempo

### Ability progression themes

`Fireball`

- Base role: ranged AoE pressure
- Tree themes:
  - larger blast or harder burst
  - burn / fire-zone aftermath
  - safer casting after hitting multiple targets

`Icebeam`

- Base role: clean control spell
- Tree themes:
  - stronger or longer slow
  - anti-mobility utility
  - multi-target frost pressure around the main target

`Lightningstrike`

- Base role: line pressure and tempo disruption
- Tree themes:
  - initiative damage
  - better single-target or line payoff
  - stronger use of no-move and element-switch windows

`Rockblast`

- Base role: terrain shaping and zone denial
- Tree themes:
  - stronger rock creation
  - anti-mobility terrain pressure
  - safer use through self-protection and chokepoint control

`ArcaneShift`

- Base role: reposition without giving up mage pressure
- Tree themes:
  - safer relocation
  - leave elemental traces behind
  - prepare the next spell or preserve existing element bonuses

## Character 5

### Role

Character 5 is a straightforward fighter / bruiser. He wants to run in, trade hits directly, build blood from damage dealt and use that blood for sustain, kill pressure and comeback power.

### Resource

Character 5 gains blood as a fraction of the damage he deals. He does not gain blood per hit.

Blood is used for:

- healing
- cheat death effects
- offensive and defensive blood-based passives

### Abilities

- `Dash`
  direct engage tool.
- `Decimate`
  heavy single-target hit with percent-health pressure.
- `SpinToWin`
  melee AoE around Character 5.
- `Heal`
  convert blood into sustain.
- `Rend`
  bleed and chase tool.

### Character progression

Character 5's character tree supports:

- higher health or raw pressure
- more blood gain
- better movement into fights
- stronger low-health fighting
- blood-based cheat death and comeback play

### Ability progression themes

`Dash`

- Base role: engage and stick tool
- Tree themes:
  - chase wounded targets
  - anti-escape pressure
  - safer all-in entry

`Decimate`

- Base role: tank breaker and finisher
- Tree themes:
  - stronger max-health scaling
  - blood gain and lifesteal
  - better payoff into low-health or overkill situations

`SpinToWin`

- Base role: stand in the middle and win trades
- Tree themes:
  - more area value
  - blood and healing from multi-target hits
  - anti-mobility pressure while fighting in the middle of the enemy team

`Heal`

- Base role: blood conversion into survival
- Tree themes:
  - stronger raw healing
  - better low-health emergency use
  - turn the heal into immediate combat readiness through damage or lifesteal windows

`Rend`

- Base role: bleed and chase
- Tree themes:
  - stronger wound over time
  - prevent the target from escaping
  - reward staying on the same wounded target

## Character 6

### Role

Character 6 is the most versatile character in the game. He controls a puppet and can shift between pressure, control, safety and coordination play depending on the situation.

`PuppetMove` is core functionality and not part of the normal ability skill trees.

### Abilities

- `PuppetSwitch`
  swap positions between Character 6 and the puppet.
- `PuppetSlash`
  AoE around the puppet.
- `PuppetHit`
  melee single-target puppet hit.
- `PuppetDefend`
  defensive / control state around the puppet.
- `StringSnare`
  bind and control target with strings.

### Character progression

Character 6's character tree supports:

- defining the puppet role
  - `Frontpuppe`
  - `Kampfpuppe`
  - `Jagdpupe`
- better puppet control range
- better use of `PuppetMove`
- more value when Character 6 and the puppet pressure the same target
- adapting to whether the puppet is protecting, engaging or setting up

### Ability progression themes

`PuppetSwitch`

- Base role: position swap and role swap
- Tree themes:
  - rescue and disengage
  - aggressive follow-up after swapping
  - stronger shared pressure when both units cover the same area

`PuppetSlash`

- Base role: AoE control around the puppet
- Tree themes:
  - anti-mobility around the puppet
  - better duo follow-up for Character 6
  - larger or more disruptive slash patterns

`PuppetHit`

- Base role: precise melee setup hit
- Tree themes:
  - target marking and armor breaking
  - anti-mobility single target control
  - major functional changes such as pulling the puppet in or knocking the target away

`PuppetDefend`

- Base role: defensive anchor and control aura
- Tree themes:
  - protect Character 6 while the puppet stands forward
  - deny movement around the puppet
  - reward attacking targets that stand in the puppet's defended space

`StringSnare`

- Base role: direct control and duo setup
- Tree themes:
  - stronger movement tax
  - better damage or control windows for Character 6 and the puppet
  - punish enemies that still try to move through the snare

## Aura system

The project now also has a reusable dynamic aura system.

It can be used for abilities that:

- apply an effect in a radius around a unit
- update dynamically when characters move
- add or remove modifiers as units enter or leave the aura

`PuppetDefend` already uses this system, and it is intended as the foundation for future aura-like abilities.

## Current design rules for skill trees

Across all characters, the current ability and character trees follow these rules:

- 4 rows per tree
- 3 choices per row
- rows should contain different kinds of decisions, not three tiny versions of the same stat buff
- earlier rows are usually simpler and easier to read
- deeper rows can contain more complex effects
- complex gameplay effects should be visible in buffs / debuffs and not be hidden from the player

## Status

Current implementation status:

- character progression trees: implemented
- ability progression trees: implemented
- fifth skill per character: implemented
- default team presets with learned skills and initial point allocations: implemented
- match recording to JSON: implemented
- dynamic aura system: implemented
