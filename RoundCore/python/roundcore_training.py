"""NumPy batching helpers for the standalone RoundCore Python module."""

from dataclasses import dataclass
from typing import Iterable

import numpy as np
import roundcore_python as rc


@dataclass(frozen=True)
class BatchShape:
    max_tiles: int
    max_characters: int
    max_abilities: int
    max_actions: int
    max_action_selections: int

    def __post_init__(self) -> None:
        for name, value in vars(self).items():
            if value < 0:
                raise ValueError(f"{name} must not be negative")


def infer_batch_shape(observations: Iterable[rc.NumericObservation]) -> BatchShape:
    observations = list(observations)
    return BatchShape(
        max_tiles=max((_records(item.tiles, rc.TILE_STRIDE) for item in observations), default=0),
        max_characters=max((_records(item.characters, rc.CHARACTER_STRIDE) for item in observations), default=0),
        max_abilities=max((_records(item.abilities, rc.ABILITY_STRIDE) for item in observations), default=0),
        max_actions=max((_records(item.legal_actions, rc.ACTION_STRIDE) for item in observations), default=0),
        max_action_selections=max(
            (_records(item.action_selections, rc.ACTION_SELECTION_STRIDE) for item in observations), default=0),
    )


def encode_batch(observations: Iterable[rc.NumericObservation], shape: BatchShape) -> dict[str, np.ndarray]:
    observations = list(observations)
    batch_size = len(observations)
    result = {
        "global": np.zeros((batch_size, rc.GLOBAL_STRIDE), dtype=np.int64),
        "tiles": np.zeros((batch_size, shape.max_tiles, rc.TILE_STRIDE), dtype=np.int64),
        "tile_mask": np.zeros((batch_size, shape.max_tiles), dtype=np.bool_),
        "characters": np.zeros((batch_size, shape.max_characters, rc.CHARACTER_STRIDE), dtype=np.int64),
        "character_mask": np.zeros((batch_size, shape.max_characters), dtype=np.bool_),
        "abilities": np.zeros((batch_size, shape.max_abilities, rc.ABILITY_STRIDE), dtype=np.int64),
        "ability_mask": np.zeros((batch_size, shape.max_abilities), dtype=np.bool_),
        "actions": np.zeros((batch_size, shape.max_actions, rc.ACTION_STRIDE), dtype=np.int64),
        "action_mask": np.zeros((batch_size, shape.max_actions), dtype=np.bool_),
        "action_selections": np.zeros(
            (batch_size, shape.max_action_selections, rc.ACTION_SELECTION_STRIDE), dtype=np.int64),
        "action_selection_mask": np.zeros((batch_size, shape.max_action_selections), dtype=np.bool_),
    }

    for index, observation in enumerate(observations):
        global_values = np.asarray(observation.global_values, dtype=np.int64)
        if global_values.size != rc.GLOBAL_STRIDE:
            raise ValueError("numeric observation has an incompatible global stride")
        result["global"][index] = global_values
        _copy_records(result, index, "tiles", "tile_mask", observation.tiles, rc.TILE_STRIDE)
        _copy_records(result, index, "characters", "character_mask", observation.characters, rc.CHARACTER_STRIDE)
        _copy_records(result, index, "abilities", "ability_mask", observation.abilities, rc.ABILITY_STRIDE)
        _copy_records(result, index, "actions", "action_mask", observation.legal_actions, rc.ACTION_STRIDE)
        _copy_records(
            result, index, "action_selections", "action_selection_mask",
            observation.action_selections, rc.ACTION_SELECTION_STRIDE)
    return result


def _records(values: list[int], stride: int) -> int:
    if len(values) % stride:
        raise ValueError(f"record data length {len(values)} is not divisible by stride {stride}")
    return len(values) // stride


def _copy_records(
    result: dict[str, np.ndarray], batch_index: int, values_key: str, mask_key: str,
    values: list[int], stride: int,
) -> None:
    record_count = _records(values, stride)
    capacity = result[values_key].shape[1]
    if record_count > capacity:
        raise ValueError(f"{values_key} needs {record_count} records but configured capacity is {capacity}")
    if record_count:
        result[values_key][batch_index, :record_count] = np.asarray(values, dtype=np.int64).reshape(record_count, stride)
        result[mask_key][batch_index, :record_count] = True
