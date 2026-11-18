import unreal


ROOT = "/Game/TopDown/Field/Terrain"
MATERIAL_ROOT = f"{ROOT}/Materials"

MESHES = {
    "cube": unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube"),
    "sphere": unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Sphere.Sphere"),
    "cylinder": unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder"),
    "cone": unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cone.Cone"),
}
if any(mesh is None for mesh in MESHES.values()):
    raise RuntimeError("One or more Engine basic meshes could not be loaded")


def material(name, color, roughness=0.82, emissive=0.0):
    path = f"{MATERIAL_ROOT}/{name}"
    asset = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if asset:
        return asset

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew()
    )
    base = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant3Vector, -320, 0
    )
    base.set_editor_property("constant", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        asset, unreal.MaterialExpressionConstant, -320, 180
    )
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    if emissive > 0.0:
        multiplier = unreal.MaterialEditingLibrary.create_material_expression(
            asset, unreal.MaterialExpressionMultiply, -80, 80
        )
        intensity = unreal.MaterialEditingLibrary.create_material_expression(
            asset, unreal.MaterialExpressionConstant, -320, 90
        )
        intensity.set_editor_property("r", emissive)
        unreal.MaterialEditingLibrary.connect_material_expressions(base, "", multiplier, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(intensity, "", multiplier, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            multiplier, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    unreal.MaterialEditingLibrary.recompile_material(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


M = {
    "sand": material("M_Terrain_Sand", unreal.LinearColor(0.72, 0.48, 0.20, 1.0)),
    "sand_light": material("M_Terrain_SandLight", unreal.LinearColor(0.95, 0.72, 0.32, 1.0)),
    "shallow": material("M_Terrain_ShallowWater", unreal.LinearColor(0.12, 0.62, 0.78, 1.0), 0.22),
    "foam": material("M_Terrain_WaterFoam", unreal.LinearColor(0.42, 0.90, 0.94, 1.0), 0.18, 0.12),
    "deep": material("M_Terrain_DeepWater", unreal.LinearColor(0.025, 0.16, 0.42, 1.0), 0.18),
    "mud": material("M_Terrain_Mud", unreal.LinearColor(0.20, 0.10, 0.035, 1.0)),
    "mud_light": material("M_Terrain_MudLight", unreal.LinearColor(0.38, 0.22, 0.08, 1.0)),
    "reed": material("M_Terrain_Reed", unreal.LinearColor(0.22, 0.48, 0.08, 1.0)),
    "reed_tip": material("M_Terrain_ReedTip", unreal.LinearColor(0.34, 0.16, 0.045, 1.0)),
    "leaf": material("M_Terrain_OasisLeaf", unreal.LinearColor(0.05, 0.46, 0.12, 1.0)),
    "trunk": material("M_Terrain_Trunk", unreal.LinearColor(0.30, 0.13, 0.035, 1.0)),
    "ash": material("M_Terrain_Ash", unreal.LinearColor(0.075, 0.065, 0.07, 1.0)),
    "ash_light": material("M_Terrain_AshLight", unreal.LinearColor(0.20, 0.17, 0.18, 1.0)),
    "lava": material("M_Terrain_Lava", unreal.LinearColor(0.95, 0.08, 0.005, 1.0), 0.24, 2.5),
    "lava_hot": material("M_Terrain_LavaHot", unreal.LinearColor(1.0, 0.46, 0.015, 1.0), 0.2, 4.0),
    "basalt": material("M_Terrain_Basalt", unreal.LinearColor(0.035, 0.035, 0.045, 1.0)),
    "stone": material("M_Terrain_Stone", unreal.LinearColor(0.31, 0.34, 0.38, 1.0)),
    "stone_light": material("M_Terrain_StoneLight", unreal.LinearColor(0.52, 0.55, 0.58, 1.0)),
    "wood": material("M_Terrain_Wood", unreal.LinearColor(0.38, 0.16, 0.045, 1.0)),
    "wood_light": material("M_Terrain_WoodLight", unreal.LinearColor(0.62, 0.32, 0.09, 1.0)),
    "meadow": material("M_Terrain_Meadow", unreal.LinearColor(0.24, 0.58, 0.10, 1.0)),
    "meadow_light": material("M_Terrain_MeadowLight", unreal.LinearColor(0.48, 0.78, 0.20, 1.0)),
    "tall_grass": material("M_Terrain_TallGrass", unreal.LinearColor(0.12, 0.38, 0.055, 1.0)),
    "hedge": material("M_Terrain_Hedge", unreal.LinearColor(0.035, 0.22, 0.025, 1.0)),
    "canyon": material("M_Terrain_CanyonFloor", unreal.LinearColor(0.48, 0.22, 0.08, 1.0)),
    "cliff": material("M_Terrain_Cliff", unreal.LinearColor(0.24, 0.13, 0.08, 1.0)),
    "fog": material("M_Terrain_Fog", unreal.LinearColor(0.58, 0.72, 0.72, 1.0), 0.35),
    "bog": material("M_Terrain_Bog", unreal.LinearColor(0.12, 0.18, 0.055, 1.0)),
    "highland": material("M_Terrain_Highland", unreal.LinearColor(0.42, 0.50, 0.18, 1.0)),
    "ridge": material("M_Terrain_Ridge", unreal.LinearColor(0.38, 0.34, 0.26, 1.0)),
}


def part(mesh, mat, location, scale, rotation=(0.0, 0.0, 0.0), shadow=True):
    result = unreal.RoundTerrainVisualPart()
    result.set_editor_property("mesh", MESHES[mesh])
    result.set_editor_property("material", M[mat])
    result.set_editor_property("location", unreal.Vector(*location))
    result.set_editor_property("rotation", unreal.Rotator(rotation[1], rotation[2], rotation[0]))
    result.set_editor_property("scale", unreal.Vector(*scale))
    result.set_editor_property("cast_shadow", shadow)
    return result


def surface(mat, height=0.06, z=2.0):
    return part("cylinder", mat, (0, 0, z), (1.92, 1.92, height), shadow=False)


def create_blueprint(name, visual_parts):
    path = f"{ROOT}/{name}"
    blueprint = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not blueprint:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.RoundCompositeTerrainActor)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ROOT, unreal.Blueprint, factory
        )
    generated_class = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not generated_class:
        raise RuntimeError(f"Could not load generated class for {path}")
    cdo = unreal.get_default_object(generated_class)
    cdo.set_editor_property("visual_parts", visual_parts)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return generated_class


blueprints = {}

blueprints["Meadow"] = create_blueprint("BP_Terrain_Meadow", [
    surface("meadow"),
    part("sphere", "meadow_light", (-42, 24, 8), (0.18, 0.34, 0.055), (0, 18, 0), False),
    part("sphere", "meadow_light", (38, -30, 8), (0.16, 0.28, 0.05), (0, -24, 0), False),
])

blueprints["TallGrass"] = create_blueprint("BP_Terrain_TallGrass", [
    surface("meadow"),
    part("cone", "tall_grass", (-42, 26, 34), (0.16, 0.16, 0.55), (0, 8, 0)),
    part("cone", "tall_grass", (0, -22, 38), (0.18, 0.18, 0.64), (0, -10, 0)),
    part("cone", "tall_grass", (42, 18, 32), (0.15, 0.15, 0.52), (0, 14, 0)),
])
blueprints["Hedge"] = create_blueprint("BP_Terrain_Hedge", [
    surface("meadow"), part("cube", "hedge", (0, 0, 48), (1.35, 0.48, 0.72)),
])
blueprints["CanyonFloor"] = create_blueprint("BP_Terrain_CanyonFloor", [
    surface("canyon"),
])
blueprints["Cliff"] = create_blueprint("BP_Terrain_Cliff", [
    surface("canyon"), part("cube", "cliff", (0, 0, 85), (1.35, 1.15, 1.25)),
])
blueprints["Fog"] = create_blueprint("BP_Terrain_Fog", [
    surface("fog", 0.035, 8), part("sphere", "fog", (0, 0, 38), (1.35, 1.35, 0.28), shadow=False),
])
blueprints["Bog"] = create_blueprint("BP_Terrain_Bog", [
    surface("bog"),
])
blueprints["Highland"] = create_blueprint("BP_Terrain_Highland", [
    surface("highland", 0.10, 12),
])
blueprints["Ridge"] = create_blueprint("BP_Terrain_Ridge", [
    surface("highland", 0.08, 8), part("cube", "ridge", (0, 0, 42), (1.30, 0.60, 0.48)),
])

blueprints["Sand"] = create_blueprint("BP_Terrain_Sand", [
    surface("sand"),
    part("sphere", "sand_light", (-45, 30, 10), (0.65, 0.30, 0.12), (0, 18, 0), False),
    part("sphere", "sand_light", (40, -35, 9), (0.55, 0.24, 0.10), (0, -22, 0), False),
])

blueprints["ShallowWater"] = create_blueprint("BP_Terrain_ShallowWater", [
    surface("shallow", 0.045, 3),
    part("cylinder", "foam", (-42, 22, 7), (0.48, 0.48, 0.012), shadow=False),
    part("cylinder", "foam", (45, -25, 7), (0.32, 0.32, 0.012), shadow=False),
])

blueprints["DeepWater"] = create_blueprint("BP_Terrain_DeepWater", [
    surface("deep", 0.045, -1),
    part("cylinder", "shallow", (-45, -20, 3), (0.30, 0.30, 0.010), shadow=False),
    part("cylinder", "shallow", (42, 30, 3), (0.24, 0.24, 0.010), shadow=False),
])

blueprints["Mud"] = create_blueprint("BP_Terrain_Mud", [
    surface("mud_light"),
    part("sphere", "mud", (-42, -20, 9), (0.55, 0.36, 0.055), shadow=False),
    part("sphere", "mud", (35, 34, 9), (0.42, 0.30, 0.050), shadow=False),
    part("sphere", "mud", (50, -44, 8), (0.24, 0.18, 0.040), shadow=False),
])

reeds = [surface("mud_light")]
for x, y, height in [(-62,-25,0.65),(-52,28,0.85),(-28,52,0.72),(38,52,0.90),(58,18,0.70),(55,-42,0.82),(-20,-58,0.76)]:
    reeds.append(part("cylinder", "reed", (x, y, 24 * height), (0.055, 0.055, 0.48 * height)))
    reeds.append(part("sphere", "reed_tip", (x, y, 49 * height), (0.08, 0.08, 0.16)))
blueprints["Reeds"] = create_blueprint("BP_Terrain_Reeds", reeds)

blueprints["Dune"] = create_blueprint("BP_Terrain_Dune", [
    surface("sand"),
    part("sphere", "sand_light", (0, 4, 22), (1.38, 0.62, 0.30), (0, 24, 0)),
    part("sphere", "sand", (-24, -20, 31), (0.82, 0.40, 0.23), (0, 24, 0)),
])

blueprints["Oasis"] = create_blueprint("BP_Terrain_Oasis", [
    surface("sand"),
    part("cylinder", "shallow", (0, 0, 7), (1.05, 0.82, 0.035), shadow=False),
    part("cylinder", "foam", (0, 0, 10), (0.58, 0.44, 0.012), shadow=False),
    part("cylinder", "trunk", (-58, 40, 38), (0.09, 0.09, 0.72), (8, 0, -14)),
    part("cone", "leaf", (-65, 44, 78), (0.50, 0.50, 0.18), (180, 0, 0)),
    part("cylinder", "trunk", (55, -38, 30), (0.075, 0.075, 0.56), (-6, 0, 18)),
    part("cone", "leaf", (59, -42, 62), (0.42, 0.42, 0.16), (180, 0, 0)),
])

blueprints["Ash"] = create_blueprint("BP_Terrain_Ash", [
    surface("ash"),
    part("sphere", "ash_light", (-48, 28, 10), (0.26, 0.22, 0.15)),
    part("sphere", "basalt", (44, -30, 12), (0.34, 0.28, 0.20)),
    part("sphere", "ash_light", (28, 55, 8), (0.18, 0.16, 0.10)),
])

blueprints["Lava"] = create_blueprint("BP_Terrain_Lava", [
    surface("lava", 0.065, 3),
    part("sphere", "lava_hot", (-42, 20, 9), (0.58, 0.18, 0.045), (0, 32, 0), False),
    part("sphere", "lava_hot", (34, -28, 9), (0.48, 0.15, 0.04), (0, -28, 0), False),
    part("sphere", "basalt", (-62, -42, 13), (0.25, 0.22, 0.19)),
    part("sphere", "basalt", (60, 45, 12), (0.22, 0.20, 0.18)),
])

blueprints["Basalt"] = create_blueprint("BP_Terrain_Basalt", [
    surface("ash"),
    part("cylinder", "basalt", (-36, -22, 48), (0.42, 0.42, 0.92)),
    part("cylinder", "basalt", (25, -32, 62), (0.48, 0.48, 1.20)),
    part("cylinder", "ash_light", (0, 32, 42), (0.38, 0.38, 0.80)),
    part("cylinder", "basalt", (52, 28, 35), (0.30, 0.30, 0.65)),
    part("cylinder", "ash_light", (-54, 38, 30), (0.28, 0.28, 0.56)),
])

stone_floor = [surface("stone")]
for x, y, light in [(-52,-38,False),(0,-38,True),(52,-38,False),(-52,15,True),(0,15,False),(52,15,True),(-26,60,False),(28,60,True)]:
    stone_floor.append(part("cube", "stone_light" if light else "stone", (x, y, 11), (0.46, 0.42, 0.08), shadow=False))
blueprints["StoneFloor"] = create_blueprint("BP_Terrain_StoneFloor", stone_floor)

bridge = [surface("deep", 0.035, -1)]
for x in [-72, -48, -24, 0, 24, 48, 72]:
    bridge.append(part("cube", "wood_light" if x % 48 == 0 else "wood", (x, 0, 18), (0.21, 1.25, 0.12)))
bridge.extend([
    part("cube", "wood", (0, 69, 38), (1.75, 0.075, 0.075)),
    part("cube", "wood", (0, -69, 38), (1.75, 0.075, 0.075)),
])
blueprints["Bridge"] = create_blueprint("BP_Terrain_Bridge", bridge)

blueprints["Gate"] = create_blueprint("BP_Terrain_Gate", [
    surface("stone"),
    part("cube", "stone_light", (0, 0, 10), (1.55, 0.48, 0.08), shadow=False),
    part("cube", "stone", (0, -62, 58), (0.42, 0.34, 1.10)),
    part("cube", "stone", (0, 62, 58), (0.42, 0.34, 1.10)),
    part("cube", "stone_light", (0, 0, 115), (0.46, 1.55, 0.28)),
])

blueprints["Mountain"] = create_blueprint("BP_Terrain_Mountain", [
    surface("stone"),
    part("cone", "stone", (-26, -8, 72), (1.15, 1.15, 1.42)),
    part("cone", "stone_light", (45, 32, 55), (0.78, 0.78, 1.08)),
    part("cone", "ash_light", (-50, 45, 43), (0.62, 0.62, 0.82)),
])

blueprints["Wall"] = create_blueprint("BP_Terrain_Wall", [
    surface("stone"),
    part("cube", "stone", (0, -52, 48), (0.52, 0.62, 0.88)),
    part("cube", "stone_light", (0, 0, 48), (0.52, 0.62, 0.88)),
    part("cube", "stone", (0, 52, 48), (0.52, 0.62, 0.88)),
    part("cube", "stone_light", (0, -26, 105), (0.52, 0.58, 0.28)),
    part("cube", "stone", (0, 29, 105), (0.52, 0.58, 0.28)),
])


terrain_assets = {
    "DA_TerrainMeadow": "Meadow",
    "DA_TerrainTallGrass": "TallGrass",
    "DA_TerrainHedge": "Hedge",
    "DA_TerrainCanyonFloor": "CanyonFloor",
    "DA_TerrainCliff": "Cliff",
    "DA_TerrainFog": "Fog",
    "DA_TerrainBog": "Bog",
    "DA_TerrainHighland": "Highland",
    "DA_TerrainRidge": "Ridge",
    "DA_TerrainSand": "Sand",
    "DA_TerrainShallowWater": "ShallowWater",
    "DA_TerrainDeepWater": "DeepWater",
    "DA_TerrainMud": "Mud",
    "DA_TerrainReeds": "Reeds",
    "DA_TerrainDune": "Dune",
    "DA_TerrainOasis": "Oasis",
    "DA_TerrainAsh": "Ash",
    "DA_TerrainLava": "Lava",
    "DA_TerrainBasalt": "Basalt",
    "DA_TerrainStoneFloor": "StoneFloor",
    "DA_TerrainBridge": "Bridge",
    "DA_TerrainGate": "Gate",
    "DA_TerrainMountain": "Mountain",
    "DA_TerrainWall": "Wall",
}
for asset_name, blueprint_key in terrain_assets.items():
    asset = unreal.EditorAssetLibrary.load_asset(f"{ROOT}/{asset_name}")
    if not asset:
        raise RuntimeError(f"Missing terrain data asset {asset_name}")
    asset.set_editor_property("terrain_actor_class", blueprints[blueprint_key])
    unreal.EditorAssetLibrary.save_loaded_asset(asset)

unreal.log(f"Created {len(blueprints)} composite terrain Blueprint classes and registered them.")
