"""Run a rendered-free match from an Unreal-exported RoundCore package."""

from __future__ import annotations

import argparse
import random
from pathlib import Path

import roundcore_python as rc
from roundcore_training import RewardConfig, SelfPlayEnvironment, choose_tactical_action


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=Path)
    parser.add_argument("--max-decisions", type=int, default=100_000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--replay", type=Path)
    parser.add_argument("--policy", choices=("random", "tactical"), default="tactical")
    args = parser.parse_args()

    rewards = RewardConfig(0.01, -0.01, 0.01, -0.01, 1.0, -1.0, 10.0, -10.0, -0.001)
    environment = SelfPlayEnvironment.from_package(args.package, rewards)
    observation, team = environment.reset()
    generator = random.Random(args.seed)
    totals = {"TeamA": 0.0, "TeamB": 0.0}

    for decision in range(args.max_decisions):
        decision_snapshot = environment.decision()
        actions = decision_snapshot.legal_actions
        if not actions:
            raise RuntimeError("RoundCore returned no legal action for a running match")
        action_index = (choose_tactical_action(decision_snapshot)
                        if args.policy == "tactical" else generator.randrange(len(actions)))
        output = environment.step(action_index)
        key = "TeamA" if output.acting_team == rc.Team.TEAM_A else "TeamB"
        totals[key] += output.reward
        observation = output.observation
        if output.terminated:
            if args.replay:
                environment.replay.save(args.replay)
            print(
                f"completed decisions={decision + 1} "
                f"team_a_reward={totals['TeamA']:.3f} team_b_reward={totals['TeamB']:.3f}"
            )
            return 0

    print(f"decision limit reached ({args.max_decisions}); the exported ruleset is not yet self-play complete")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
