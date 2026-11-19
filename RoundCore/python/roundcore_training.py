"""NumPy batching helpers for the standalone RoundCore Python module."""

from dataclasses import asdict, dataclass
import json
from pathlib import Path
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


@dataclass(frozen=True)
class RewardConfig:
    damage_dealt: float
    damage_taken: float
    healing_done: float
    healing_received: float
    enemy_death: float
    ally_death: float
    win: float
    loss: float
    decision: float


@dataclass
class ReplayTransition:
    before_hash: int
    action_index: int
    after_hash: int


@dataclass
class ReplayTrace:
    format_version: int
    content_version: str
    initial_hash: int
    transitions: list[ReplayTransition]

    def save(self, path: str | Path) -> None:
        Path(path).write_text(json.dumps(asdict(self), indent=2), encoding="utf-8")

    @staticmethod
    def load(path: str | Path) -> "ReplayTrace":
        data = json.loads(Path(path).read_text(encoding="utf-8"))
        if data.get("format_version") != 1:
            raise ValueError(f"unsupported replay format version: {data.get('format_version')}")
        data["transitions"] = [ReplayTransition(**item) for item in data["transitions"]]
        return ReplayTrace(**data)


class ReplayRecorder:
    def __init__(self, content_version: str, initial_hash: int) -> None:
        self.trace = ReplayTrace(1, content_version, initial_hash, [])

    def record(self, before_hash: int, action_index: int, after_hash: int) -> None:
        expected = self.trace.initial_hash if not self.trace.transitions else self.trace.transitions[-1].after_hash
        if before_hash != expected:
            raise ValueError(f"replay transition starts at {before_hash}, expected {expected}")
        self.trace.transitions.append(ReplayTransition(before_hash, action_index, after_hash))


def replay_and_verify(game: rc.Game, trace: ReplayTrace) -> None:
    if game.content_version() != trace.content_version:
        raise ValueError(f"replay content mismatch: {game.content_version()} != {trace.content_version}")
    if game.state_hash() != trace.initial_hash:
        raise ValueError(f"replay initial hash mismatch: {game.state_hash()} != {trace.initial_hash}")
    for index, transition in enumerate(trace.transitions):
        if game.state_hash() != transition.before_hash:
            raise ValueError(f"replay hash mismatch before transition {index}")
        result = game.apply_decision_index(transition.action_index)
        if not result.success:
            raise ValueError(f"replay action {index} failed: {result.error_code}")
        if game.state_hash() != transition.after_hash:
            raise ValueError(f"replay hash mismatch after transition {index}")


def compute_reward(
    result: rc.ActionResult,
    before: rc.DecisionSnapshot,
    after: rc.DecisionSnapshot,
    perspective: rc.Team,
    config: RewardConfig,
) -> float:
    teams = {character.id: character.team for character in before.state.characters}
    teams.update({character.id: character.team for character in after.state.characters})
    reward = config.decision
    for event in result.events:
        source_team = teams.get(event.source_id, rc.Team.NEUTRAL)
        target_team = teams.get(event.target_id, rc.Team.NEUTRAL)
        if event.type == rc.EventType.DAMAGE_APPLIED:
            if source_team == perspective:
                reward += config.damage_dealt * event.amount
            if target_team == perspective:
                reward += config.damage_taken * event.amount
        elif event.type == rc.EventType.HEALING_APPLIED:
            if source_team == perspective:
                reward += config.healing_done * event.amount
            if target_team == perspective:
                reward += config.healing_received * event.amount
        elif event.type == rc.EventType.CHARACTER_DIED:
            reward += config.ally_death if target_team == perspective else config.enemy_death
    if after.state.status == rc.MatchStatus.COMPLETED:
        if after.state.winner == perspective:
            reward += config.win
        elif after.state.winner != rc.Team.NEUTRAL:
            reward += config.loss
    return reward


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
