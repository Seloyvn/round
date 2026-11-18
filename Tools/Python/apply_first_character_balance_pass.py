import unreal


def _text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def _set_stats(asset, stats_data):
    stats = asset.get_editor_property("base_stats")
    for key, value in stats_data.items():
        setattr(stats, key, value)
    asset.set_editor_property("base_stats", stats)


def _save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save asset: {asset.get_path_name()}")


def _load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"missing asset: {path}")
    return asset


CHARACTERS = {
    "/Game/TopDown/Characters/Engineer/EngineerDetails": {
        "stats": {
            "max_health": 1850,
            "attack": 110,
            "defense": 100,
            "max_initiative": 95,
            "vision_range": 4,
            "attack_range": 3,
            "move_cost": 17,
            "attack_cost": 15,
            "turn_delay": 5,
        },
        "details": {
            "MaxDevices": 3.0,
        },
    },
    "/Game/TopDown/Characters/Chronomancer/ChronomancerDetails": {
        "stats": {
            "max_health": 1500,
            "attack": 100,
            "defense": 80,
            "max_initiative": 85,
            "vision_range": 4,
            "attack_range": 4,
            "move_cost": 13,
            "attack_cost": 13,
            "turn_delay": 4,
        },
        "details": {
            "MaxFragments": 5.0,
        },
    },
    "/Game/TopDown/Characters/Druid/DruidDetails": {
        "stats": {
            "max_health": 1750,
            "attack": 90,
            "defense": 110,
            "max_initiative": 88,
            "vision_range": 4,
            "attack_range": 3,
            "move_cost": 14,
            "attack_cost": 13,
            "turn_delay": 4,
        },
        "details": {},
    },
    "/Game/TopDown/Characters/Dragoon/DragoonDetails": {
        "stats": {
            "max_health": 2150,
            "attack": 170,
            "defense": 125,
            "max_initiative": 120,
            "vision_range": 4,
            "attack_range": 2,
            "move_cost": 13,
            "attack_cost": 15,
            "turn_delay": 10,
        },
        "details": {
            "MomentumMaxStacks": 4.0,
            "DamagePerStack": 25.0,
        },
    },
    "/Game/TopDown/Characters/Geomancer/GeomancerDetails": {
        "stats": {
            "max_health": 1900,
            "attack": 140,
            "defense": 130,
            "max_initiative": 110,
            "vision_range": 4,
            "attack_range": 1,
            "move_cost": 17,
            "attack_cost": 16,
            "turn_delay": 9,
        },
        "details": {
            "MarkedAttackBonusDamage": 70.0,
            "PassivePerTargetCooldownInitiative": 70.0,
        },
    },
    "/Game/TopDown/Characters/Gambler/GamblerDetails": {
        "stats": {
            "max_health": 1400,
            "attack": 125,
            "defense": 70,
            "max_initiative": 110,
            "vision_range": 4,
            "attack_range": 3,
            "move_cost": 13,
            "attack_cost": 13,
            "turn_delay": 8,
        },
        "details": {
            "MaxHandCards": 5.0,
        },
    },
}


ABILITIES = {
    "/Game/TopDown/Data/Abilities/DA_Ability_Turret": {
        "initiative_cost": 25,
        "cooldown": 300,
        "range": 4,
        "base_damage": 130,
        "values": {
            "DurationInitiative": 200.0,
            "Health": 220.0,
            "TurretRange": 4.0,
            "TriggerIntervalInitiative": 50.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Mine": {
        "initiative_cost": 20,
        "cooldown": 240,
        "range": 4,
        "base_damage": 250,
        "values": {
            "DurationInitiative": 220.0,
            "Health": 80.0,
            "AOERange": 1.0,
            "ArmDelayInitiative": 30.0,
            "EnemyRevealInitiative": 15.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_ShieldEmitter": {
        "initiative_cost": 35,
        "cooldown": 360,
        "range": 4,
        "base_damage": 0,
        "values": {
            "DurationInitiative": 200.0,
            "Health": 170.0,
            "ShieldAmount": 100.0,
            "AuraRange": 2.0,
            "ShieldDurationInitiative": 50.0,
            "TriggerIntervalInitiative": 50.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_SpotterBeacon": {
        "initiative_cost": 25,
        "cooldown": 260,
        "range": 4,
        "base_damage": 0,
        "values": {
            "DurationInitiative": 240.0,
            "Health": 130.0,
            "AuraRange": 3.0,
            "DamageTakenMultiplier": 1.15,
            "DebuffDurationInitiative": 45.0,
            "TriggerIntervalInitiative": 40.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Overload": {
        "initiative_cost": 30,
        "cooldown": 320,
        "range": 4,
        "base_damage": 0,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Accelerate": {
        "initiative_cost": 25,
        "cooldown": 280,
        "range": 4,
        "base_damage": 0,
        "values": {
            "CooldownReduction": 50.0,
            "FragmentCost": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_BorrowedTime": {
        "initiative_cost": 25,
        "cooldown": 280,
        "range": 4,
        "base_damage": 0,
        "values": {
            "DurationInitiative": 100.0,
            "DeferredDamageFraction": 0.45,
            "FragmentCost": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Stasis": {
        "initiative_cost": 35,
        "cooldown": 340,
        "range": 4,
        "base_damage": 0,
        "values": {
            "DurationInitiative": 70.0,
            "FragmentCost": 2.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_TemporalAnchor": {
        "initiative_cost": 25,
        "cooldown": 300,
        "range": 4,
        "base_damage": 0,
        "values": {
            "DurationInitiative": 110.0,
            "FragmentCost": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_TimeSiphon": {
        "initiative_cost": 10,
        "cooldown": 180,
        "range": 4,
        "base_damage": 0,
        "values": {
            "InitiativeDrain": 35.0,
            "FragmentsGained": 2.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_ThornPatch": {
        "initiative_cost": 20,
        "cooldown": 160,
        "range": 3,
        "base_damage": 120,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Grove": {
        "initiative_cost": 20,
        "cooldown": 160,
        "range": 3,
        "base_damage": 0,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Rootsnare": {
        "initiative_cost": 25,
        "cooldown": 220,
        "range": 3,
        "base_damage": 100,
        "values": {
            "DurationInitiative": 45.0,
            "BonusDurationOnNatureInitiative": 35.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_WildBloom": {
        "initiative_cost": 40,
        "cooldown": 320,
        "range": 0,
        "base_damage": 0,
        "values": {
            "ThornDamage": 160.0,
            "GroveHeal": 140.0,
            "ForestInitiativeGain": 15.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_BearForm": {
        "initiative_cost": 0,
        "cooldown": 700,
        "range": 0,
        "base_damage": 0,
        "values": {
            "BearMaxHealth": 900.0,
            "BearAttack": 210.0,
            "BearDefense": 130.0,
            "BearMoveCost": 13.0,
            "BearAttackCost": 17.0,
            "BearTurnDelay": 9.0,
            "BearMaxInitiative": 105.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Lunge": {
        "initiative_cost": 20,
        "cooldown": 240,
        "range": 5,
        "base_damage": 170,
        "values": {
            "BehindDistance": 2.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Overshoot": {
        "initiative_cost": 30,
        "cooldown": 280,
        "range": 4,
        "base_damage": 210,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_ExecuteDive": {
        "initiative_cost": 15,
        "cooldown": 160,
        "range": 4,
        "base_damage": 150,
        "values": {
            "LowHealthThresholdPercent": 0.35,
            "DamageMultiplierVsLowHealth": 1.8,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_HoldtheLine": {
        "initiative_cost": 25,
        "cooldown": 320,
        "range": 0,
        "base_damage": 170,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Repel": {
        "initiative_cost": 20,
        "cooldown": 200,
        "range": 2,
        "base_damage": 130,
        "values": {
            "PushDistance": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_StoneWall": {
        "initiative_cost": 20,
        "cooldown": 160,
        "range": 4,
        "base_damage": 0,
        "values": {
            "WallLength": 3.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Faultline": {
        "initiative_cost": 25,
        "cooldown": 220,
        "range": 4,
        "base_damage": 120,
        "values": {
            "RockBonusDamage": 110.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Detonate": {
        "initiative_cost": 25,
        "cooldown": 220,
        "range": 4,
        "base_damage": 130,
        "values": {
            "AOERange": 1.0,
            "ExplosionRadius": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Tremor": {
        "initiative_cost": 25,
        "cooldown": 240,
        "range": 0,
        "base_damage": 110,
        "values": {
            "AOERange": 1.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Landsplitter": {
        "initiative_cost": 35,
        "cooldown": 300,
        "range": 4,
        "base_damage": 160,
        "values": {},
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_QuickDraw": {
        "initiative_cost": 15,
        "cooldown": 120,
        "range": 4,
        "base_damage": 75,
        "values": {
            "HeartsHeal": 35.0,
            "ClubsInitiativeLoss": 16.0,
            "DiamondsInitiativeGain": 16.0,
            "SpadesBonusDamage": 35.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_DoubleDraw": {
        "initiative_cost": 15,
        "cooldown": 180,
        "range": 0,
        "base_damage": 0,
        "values": {
            "DrawCount": 2.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_LowStakes": {
        "initiative_cost": 15,
        "cooldown": 220,
        "range": 0,
        "base_damage": 0,
        "values": {
            "MinimumKeptRank": 10.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_Flush": {
        "initiative_cost": 30,
        "cooldown": 300,
        "range": 4,
        "base_damage": 65,
        "values": {
            "HeartsHeal": 60.0,
            "ClubsInitiativeLoss": 16.0,
            "DiamondsInitiativeGain": 18.0,
            "DefenseIgnore": 18.0,
        },
    },
    "/Game/TopDown/Data/Abilities/DA_Ability_RigTheDeck": {
        "initiative_cost": 20,
        "cooldown": 260,
        "range": 0,
        "base_damage": 0,
        "values": {},
    },
}


for path, payload in CHARACTERS.items():
    asset = _load(path)
    _set_stats(asset, payload["stats"])
    asset.set_editor_property("details", payload["details"])
    _save(asset)
    unreal.log(f"[apply_first_character_balance_pass] saved character {path}")

for path, payload in ABILITIES.items():
    asset = _load(path)
    asset.set_editor_property("initiative_cost", payload["initiative_cost"])
    asset.set_editor_property("cooldown", payload["cooldown"])
    asset.set_editor_property("range", payload["range"])
    asset.set_editor_property("base_damage", payload["base_damage"])
    asset.set_editor_property("values", payload["values"])
    _save(asset)
    unreal.log(f"[apply_first_character_balance_pass] saved ability {path}")

unreal.log("[apply_first_character_balance_pass] done")
