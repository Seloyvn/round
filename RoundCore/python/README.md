# RoundCore Python module

The Python module is an optional standalone build. It is not linked into Unreal.

```powershell
python -m pip install -r RoundCore/python/requirements.txt
cmake -S RoundCore -B RoundCore/build-python -DROUNDCORE_BUILD_PYTHON=ON
cmake --build RoundCore/build-python --config Release
ctest --test-dir RoundCore/build-python -C Release --output-on-failure
```

The build output contains `roundcore_python` and `roundcore_training.py`. The native module exposes the authoritative Core environment. `roundcore_training.encode_batch` pads numeric observations to caller-provided capacities and emits masks for every variable-length record collection.

`RewardConfig` keeps training rewards explicit and separate from gameplay rules. `ReplayRecorder` stores action indices and before/after state hashes; `replay_and_verify` fails on the first deterministic mismatch. A replay intentionally does not embed balance content—the caller must load the matching versioned content package and reset the game before verification.

`SelfPlayEnvironment.from_package(...)` loads a resolved match package exported by Unreal and exposes a compact `reset`/`step` loop. It always returns the visibility-filtered numeric observation for the team that owns the current decision, accepts only an index from Core's legal-action list, calculates rewards from emitted events, and records a deterministic replay trace. `choose_aggressive_action` is a deterministic non-learning baseline for initial evaluation; it does not implement gameplay rules.

`observe_numeric` remains available for debugging with omniscient state. Training policies should use `observe_team_numeric` or `SelfPlayEnvironment`, which filter hidden tiles, enemy characters, seed, and random-stream state.

To verify a real exported setup without rendering, build the Python module and run:

```powershell
python RoundCore/build-python/Release/run_package_smoke.py Saved/Training/roundcore_match.rndcore --replay Saved/Training/smoke_replay.json
```

The smoke runner deliberately selects random legal actions. Reaching its decision limit indicates that the exported ruleset still contains a gameplay dependency that prevents a complete standalone match; it is a migration diagnostic, not a learning algorithm.
