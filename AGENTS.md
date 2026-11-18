# Agent Notes

Primary project guidance lives in [CODEX.md](./CODEX.md).

## Quick Rules

- Actor logic is authoritative.
- Do not reintroduce sim-based gameplay execution.
- Use registries and asset references instead of hardcoded file paths.
- Use additive key-values for ability scaling.
- Prefer child classes for specialized abilities, auras, and effects.
- Follow the skillpoint design guidelines in `CODEX.md`: one choice per row, sequential row unlocking, mixed option types, and character/skill identity over rigid formulas.
- Always analyze and summarize a character or ability's current purpose, gameplay use, limitations, and suitable tree goals before proposing its skillpoints.
