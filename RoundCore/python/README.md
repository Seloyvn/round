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

`NumericObservation` is currently omniscient. Do not train a final player policy against it until visibility and hidden-information filtering are Core-authoritative.
