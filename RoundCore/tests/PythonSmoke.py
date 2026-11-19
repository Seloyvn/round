import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(sys.argv[1]).resolve()))
import roundcore_python as rc
from roundcore_training import (
    BatchShape,
    ReplayRecorder,
    ReplayTrace,
    RewardConfig,
    compute_reward,
    encode_batch,
    infer_batch_shape,
    replay_and_verify,
)


def build_match(seed: int) -> rc.MatchSetup:
    terrain = rc.TerrainDefinition()
    terrain.id = "Arena"

    stats = rc.StatBlock()
    stats.max_health = 10
    stats.attack = 10
    stats.max_initiative = 20
    stats.attack_range = 1
    stats.move_cost = 2
    stats.attack_cost = 5
    stats.turn_delay = 5
    fighter = rc.CharacterDefinition()
    fighter.id = "Fighter"
    fighter.stats = stats

    definitions = rc.ContentDefinitions()
    definitions.content_version = "python-smoke-v1"
    definitions.terrains = {"Arena": terrain}
    definitions.characters = {"Fighter": fighter}

    first_tile = rc.TileState()
    first_tile.id = 1
    first_tile.terrain_id = "Arena"
    second_tile = rc.TileState()
    second_tile.id = 2
    second_tile.coord.q = 1
    second_tile.terrain_id = "Arena"

    first = rc.CharacterState()
    first.id = 100
    first.definition_id = "Fighter"
    first.team = rc.Team.TEAM_A
    first.tile_id = 1
    first.health = 10
    first.initiative = 20
    second = rc.CharacterState()
    second.id = 200
    second.definition_id = "Fighter"
    second.team = rc.Team.TEAM_B
    second.tile_id = 2
    second.health = 10
    second.initiative = 20

    state = rc.GameState()
    state.match_id = seed
    state.seed = seed
    state.active_character_id = 100
    state.tiles = [first_tile, second_tile]
    state.characters = [first, second]

    setup = rc.MatchSetup()
    setup.definitions = definitions
    setup.initial_state = state
    return setup


game = rc.Game()
assert game.reset(build_match(1)).success
observation = game.observe_decision()
attack_index = next(i for i, action in enumerate(observation.legal_actions)
                    if action.type == rc.ActionType.BASIC_ATTACK)
before_hash = game.state_hash()
attack_result = game.apply_decision_index(attack_index)
assert attack_result.success
after = game.observe_decision()
assert game.is_terminal()
reward = compute_reward(
    attack_result, observation, after, rc.Team.TEAM_A,
    RewardConfig(0.1, -0.1, 0.0, 0.0, 2.0, -2.0, 10.0, -10.0, -0.01),
)
assert abs(reward - 12.99) < 1e-9

recorder = ReplayRecorder("python-smoke-v1", before_hash)
recorder.record(before_hash, attack_index, game.state_hash())
replay_game = rc.Game()
assert replay_game.reset(build_match(1)).success
replay_and_verify(replay_game, recorder.trace)

environments = rc.VectorEnvironment()
assert all(result.success for result in environments.reset([build_match(i) for i in range(2, 10)]))
observations = environments.observe()
indices = [next(i for i, action in enumerate(observation.legal_actions)
                if action.type == rc.ActionType.BASIC_ATTACK)
           for observation in observations]
step = environments.step(indices)
assert all(result.success for result in step.results)
assert all(observation.state.status == rc.MatchStatus.COMPLETED for observation in step.observations)

schema = rc.build_observation_schema(build_match(99).definitions)
numeric = environments.observe_numeric(schema)
assert len(numeric) == 8
assert all(observation.content_version == "python-smoke-v1" for observation in numeric)
shape = infer_batch_shape(numeric)
batch = encode_batch(numeric, shape)
assert batch["global"].shape == (8, rc.GLOBAL_STRIDE)
assert batch["tiles"].shape == (8, 2, rc.TILE_STRIDE)
assert batch["tile_mask"].all()
assert not batch["action_mask"].any()  # All matches above are terminal.
try:
    encode_batch(numeric, BatchShape(1, 2, 0, 0, 0))
except ValueError as error:
    assert "tiles needs 2 records" in str(error)
else:
    raise AssertionError("batch capacity overflow must be rejected")

replay_path = pathlib.Path(sys.argv[1]) / "python_smoke_replay.json"
recorder.trace.save(replay_path)
loaded_replay = ReplayTrace.load(replay_path)
assert loaded_replay.transitions == recorder.trace.transitions
replay_path.unlink()
print("RoundCore Python smoke test passed")
