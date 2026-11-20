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
    max_modifier_effects: int
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


def load_match_package(path: str | Path) -> rc.MatchSetup:
    """Load the exact resolved match setup exported by Unreal."""
    return rc.deserialize_match_setup(Path(path).read_bytes())


@dataclass(frozen=True)
class StepOutput:
    observation: rc.NumericObservation
    reward: float
    terminated: bool
    acting_team: rc.Team
    result: rc.ActionResult


class SelfPlayEnvironment:
    """Small training facade over the authoritative native RoundCore game.

    The policy always receives the observation filtered for the team that owns
    the current decision. Actions are indices into that observation's legal
    action list, so a trainer cannot bypass Core validation.
    """

    def __init__(self, setup: rc.MatchSetup, reward_config: RewardConfig) -> None:
        self._setup = setup
        self._reward_config = reward_config
        self._schema = rc.build_observation_schema(setup.definitions)
        self._game = rc.Game()
        self._recorder: ReplayRecorder | None = None

    @classmethod
    def from_package(cls, path: str | Path, reward_config: RewardConfig) -> "SelfPlayEnvironment":
        return cls(load_match_package(path), reward_config)

    @property
    def replay(self) -> ReplayTrace:
        if self._recorder is None:
            raise RuntimeError("reset must be called before reading the replay")
        return self._recorder.trace

    def reset(self) -> tuple[rc.NumericObservation, rc.Team]:
        result = self._game.reset(self._setup)
        if not result.success:
            raise ValueError(f"RoundCore reset failed: {result.error_code}: {result.message}")
        while not self._game.is_terminal() and self._game.observe_decision().state.active_character_id == 0:
            advance = self._game.apply_decision_index(0)
            if not advance.success:
                raise RuntimeError(
                    f"RoundCore could not advance to the first decision: {advance.error_code}: {advance.message}")
        self._recorder = ReplayRecorder(self._game.content_version(), self._game.state_hash())
        team = self._acting_team()
        return self._game.observe_team_numeric(team, self._schema), team

    def legal_actions(self) -> list[rc.GameAction]:
        team = self._acting_team()
        return self._game.observe_team_decision(team).legal_actions

    def decision(self) -> rc.DecisionSnapshot:
        return self._game.observe_team_decision(self._acting_team())

    def step(self, action_index: int) -> StepOutput:
        if self._recorder is None:
            raise RuntimeError("reset must be called before step")
        acting_team = self._acting_team()
        before = self._game.observe_team_decision(acting_team)
        if action_index < 0 or action_index >= len(before.legal_actions):
            raise IndexError(f"legal action index {action_index} is outside [0, {len(before.legal_actions)})")
        before_hash = self._game.state_hash()
        result = self._game.apply_decision_index(action_index)
        if not result.success:
            raise RuntimeError(f"RoundCore rejected a listed legal action: {result.error_code}: {result.message}")
        after = self._game.observe_team_decision(acting_team)
        after_hash = self._game.state_hash()
        self._recorder.record(before_hash, action_index, after_hash)
        reward = compute_reward(result, before, after, acting_team, self._reward_config)
        next_team = acting_team if self._game.is_terminal() else self._acting_team()
        observation = self._game.observe_team_numeric(next_team, self._schema)
        return StepOutput(observation, reward, self._game.is_terminal(), acting_team, result)

    def _acting_team(self) -> rc.Team:
        snapshot = self._game.observe_decision()
        active_id = snapshot.state.active_character_id
        for character in snapshot.state.characters:
            if character.id == active_id:
                return character.team
        if snapshot.state.status == rc.MatchStatus.COMPLETED:
            return snapshot.state.winner
        raise RuntimeError("running match has no active decision team")


def choose_aggressive_action(actions: list[rc.GameAction]) -> int:
    """Deterministic non-learning baseline: ability, attack, move, then end turn."""
    priorities = {
        rc.ActionType.USE_ABILITY: 0,
        rc.ActionType.BASIC_ATTACK: 1,
        rc.ActionType.MOVE: 2,
        rc.ActionType.CLAIM_OBJECTIVE: 3,
        rc.ActionType.WAIT: 4,
        rc.ActionType.END_TURN: 5,
    }
    if not actions:
        raise ValueError("cannot choose from an empty legal-action list")
    return min(range(len(actions)), key=lambda index: (priorities[actions[index].type], index))


def choose_tactical_action(decision: rc.DecisionSnapshot) -> int:
    """Deterministic baseline that attacks, approaches visible enemies, or explores toward map center."""
    actions = decision.legal_actions
    if not actions:
        raise ValueError("cannot choose from an empty legal-action list")
    for action_type in (rc.ActionType.USE_ABILITY, rc.ActionType.BASIC_ATTACK):
        for index, action in enumerate(actions):
            if action.type == action_type:
                return index

    active_id = decision.state.active_character_id
    active = next(character for character in decision.state.characters if character.id == active_id)
    tiles = {tile.id: tile for tile in decision.state.tiles}
    enemies = [
        character for character in decision.state.characters
        if character.alive and character.team != active.team and character.team != rc.Team.NEUTRAL
    ]
    enemy_tiles = [tiles[character.tile_id] for character in enemies if character.tile_id in tiles]

    move_indices = [index for index, action in enumerate(actions) if action.type == rc.ActionType.MOVE]
    if move_indices:
        def score(index: int) -> tuple[int, int]:
            destination = tiles[actions[index].target_tile_id]
            if enemy_tiles:
                distance = min(_hex_distance(
                    destination.coord.q, destination.coord.r, enemy.coord.q, enemy.coord.r) for enemy in enemy_tiles)
            else:
                distance = _hex_distance(destination.coord.q, destination.coord.r, 0, 0)
            return distance, index

        return min(move_indices, key=score)
    return choose_aggressive_action(actions)


def _hex_distance(first_q: int, first_r: int, second_q: int, second_r: int) -> int:
    delta_q = first_q - second_q
    delta_r = first_r - second_r
    delta_s = (first_q + first_r) - (second_q + second_r)
    return (abs(delta_q) + abs(delta_r) + abs(delta_s)) // 2


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
        max_modifier_effects=max(
            (_records(item.modifier_effects, rc.MODIFIER_EFFECT_STRIDE) for item in observations), default=0),
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
        "modifier_effects": np.zeros(
            (batch_size, shape.max_modifier_effects, rc.MODIFIER_EFFECT_STRIDE), dtype=np.int64),
        "modifier_effect_mask": np.zeros((batch_size, shape.max_modifier_effects), dtype=np.bool_),
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
        _copy_records(
            result, index, "modifier_effects", "modifier_effect_mask",
            observation.modifier_effects, rc.MODIFIER_EFFECT_STRIDE)
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
