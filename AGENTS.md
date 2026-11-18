# Agent Notes

Primary project guidance lives in [CODEX.md](./CODEX.md).

## Quick Rules

- The standalone `RoundCore` gameplay state and rules are the migration target and will become authoritative.
- Do not build a second, approximate simulation beside the real game. Migrate one rule at a time into `RoundCore`, then make Actors call and present that rule.
- During migration, an unmigrated rule may remain Actor-authoritative. A migrated rule must not keep a second Actor-side implementation.
- Use registries and asset references instead of hardcoded file paths.
- Use additive key-values for ability scaling.
- Prefer child classes for specialized abilities, auras, and effects.
- Follow the skillpoint design guidelines in `CODEX.md`: one choice per row, sequential row unlocking, mixed option types, and character/skill identity over rigid formulas.
- Always analyze and summarize a character or ability's current purpose, gameplay use, limitations, and suitable tree goals before proposing its skillpoints.
