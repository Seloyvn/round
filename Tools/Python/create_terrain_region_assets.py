import unreal

REGION_ROOT = "/Game/TopDown/Data/TerrainRegions"
TERRAIN_ROOT = "/Game/TopDown/Field/Terrain"

def create_or_load(name, root, asset_class):
    path = f"{root}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if existing: return existing
    factory = unreal.DataAssetFactory(); factory.set_editor_property("data_asset_class", asset_class)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, root, asset_class, factory)

def terrain(name, terrain_type, display, description, actor_path, values):
    asset = create_or_load(name, TERRAIN_ROOT, unreal.RoundTerrainData)
    asset.set_editor_property("terrain_type", terrain_type); asset.set_editor_property("display_name", unreal.Text(display))
    asset.set_editor_property("description", unreal.Text(description)); asset.set_editor_property("terrain_actor_class", unreal.load_class(None, actor_path))
    asset.set_editor_property("default_weight", 0.0); asset.set_editor_property("cluster_size_min", 1); asset.set_editor_property("cluster_size_max", 1)
    asset.set_editor_property("values", values); unreal.EditorAssetLibrary.save_loaded_asset(asset); return asset

V = {"ground":"/Game/TopDown/Field/Terrain/BP_Ground.BP_Ground_C", "forest":"/Game/TopDown/Field/Terrain/BP_Forest.BP_Forest_C",
     "grove":"/Game/TopDown/Field/Terrain/BP_Grove.BP_Grove_C", "street":"/Game/TopDown/Field/Terrain/BP_Street.BP_Street_C",
     "rubble":"/Game/TopDown/Field/Terrain/BP_Rubble.BP_Rubble_C", "rocks":"/Game/TopDown/Field/Terrain/BP_Rocks.BP_Rocks_C",
     "fire":"/Game/TopDown/Field/Terrain/BP_Fire.BP_Fire_C"}
NEW_V = {name: f"/Game/TopDown/Field/Terrain/BP_Terrain_{name}.BP_Terrain_{name}_C" for name in (
    "Sand", "ShallowWater", "DeepWater", "Mud", "Reeds", "Dune", "Oasis", "Ash", "Lava", "Basalt",
    "StoneFloor", "Bridge", "Gate", "Mountain", "Wall", "Meadow")}
NEW_V.update({name: f"/Game/TopDown/Field/Terrain/BP_Terrain_{name}.BP_Terrain_{name}_C" for name in (
    "TallGrass", "Hedge", "CanyonFloor", "Cliff", "Fog", "Bog", "Highland", "Ridge")})
T = unreal.RoundTerrainType
new_terrains = {
 T.MEADOW: terrain("DA_TerrainMeadow",T.MEADOW,"Wiese","Begehbarer Untergrund ohne besonderen Effekt.",NEW_V["Meadow"],{"Moveable":1.0,"MoveCostMultiplier":1.0}),
 T.DENSE_FOREST: terrain("DA_TerrainDenseForest",T.DENSE_FOREST,"Dichter Wald","Nicht betretbare Baumgruppe, die Sicht blockiert.",V["forest"],{"Moveable":0.0,"BlocksVision":1.0}),
 T.TALL_GRASS: terrain("DA_TerrainTallGrass",T.TALL_GRASS,"Hohes Gras","Begehbar, verlangsamt leicht und verkürzt Sichtlinien durch das Feld.",NEW_V["TallGrass"],{"Moveable":1.0,"MoveCostMultiplier":1.15,"VisionCost":2.0}),
 T.HEDGE: terrain("DA_TerrainHedge",T.HEDGE,"Hecke","Nicht betretbar und sichtblockierend.",NEW_V["Hedge"],{"Moveable":0.0,"BlocksVision":1.0}),
 T.CANYON_FLOOR: terrain("DA_TerrainCanyonFloor",T.CANYON_FLOOR,"Schluchtboden","Offener, neutral begehbarer Schluchtboden.",NEW_V["CanyonFloor"],{"Moveable":1.0,"MoveCostMultiplier":1.0}),
 T.CLIFF: terrain("DA_TerrainCliff",T.CLIFF,"Felswand","Nicht betretbar und sichtblockierend.",NEW_V["Cliff"],{"Moveable":0.0,"BlocksVision":1.0}),
 T.FOG: terrain("DA_TerrainFog",T.FOG,"Nebel","Begehbar; verkürzt Sichtlinien und die Sichtweite auf dem Feld.",NEW_V["Fog"],{"Moveable":1.0,"VisionCost":2.0,"VisionRangeModifier":-1.0}),
 T.BOG: terrain("DA_TerrainBog",T.BOG,"Moorboden","Stark verlangsamend; beendet längere Bewegung beim Betreten.",NEW_V["Bog"],{"Moveable":1.0,"MoveCostMultiplier":1.65,"StopsMovement":1.0}),
 T.HIGHLAND: terrain("DA_TerrainHighland",T.HIGHLAND,"Hochland","Leicht anstrengend, gewährt aber zusätzliche Sichtweite.",NEW_V["Highland"],{"Moveable":1.0,"MoveCostMultiplier":1.2,"VisionRangeModifier":1.0}),
 T.RIDGE: terrain("DA_TerrainRidge",T.RIDGE,"Höhenrücken","Teuer begehbar, sichtblockierend und gewährt vom Feld aus stark erhöhte Sicht.",NEW_V["Ridge"],{"Moveable":1.0,"MoveCostMultiplier":1.5,"BlocksVision":1.0,"VisionRangeModifier":2.0}),
 T.SAND: terrain("DA_TerrainSand",T.SAND,"Sand","Leicht verlangsamender, offener Untergrund.",NEW_V["Sand"],{"Moveable":1.0,"MoveCostMultiplier":1.2}),
 T.SHALLOW_WATER: terrain("DA_TerrainShallowWater",T.SHALLOW_WATER,"Seichtes Wasser","Begehbares Wasser mit hohen Bewegungskosten.",NEW_V["ShallowWater"],{"Moveable":1.0,"MoveCostMultiplier":1.55}),
 T.DEEP_WATER: terrain("DA_TerrainDeepWater",T.DEEP_WATER,"Tiefes Wasser","Nicht begehbar, blockiert aber keine Sicht.",NEW_V["DeepWater"],{"Moveable":0.0,"BlocksVision":0.0}),
 T.MUD: terrain("DA_TerrainMud",T.MUD,"Schlamm","Stark verlangsamender Untergrund.",NEW_V["Mud"],{"Moveable":1.0,"MoveCostMultiplier":1.65}),
 T.REEDS: terrain("DA_TerrainReeds",T.REEDS,"Schilf","Begehbare dichte Vegetation, die Sicht blockiert.",NEW_V["Reeds"],{"Moveable":1.0,"MoveCostMultiplier":1.2,"BlocksVision":1.0}),
 T.DUNE: terrain("DA_TerrainDune",T.DUNE,"Düne","Begehbare Erhebung, die Sicht blockiert.",NEW_V["Dune"],{"Moveable":1.0,"BaseMoveCost":1.0,"MoveCostMultiplier":1.35,"BlocksVision":1.0}),
 T.OASIS: terrain("DA_TerrainOasis",T.OASIS,"Oase","Heilt zu Beginn des Zuges.",NEW_V["Oasis"],{"Moveable":1.0,"TurnStartHeal":80.0}),
 T.ASH: terrain("DA_TerrainAsh",T.ASH,"Asche","Leicht verlangsamender vulkanischer Boden.",NEW_V["Ash"],{"Moveable":1.0,"MoveCostMultiplier":1.2}),
 T.LAVA: terrain("DA_TerrainLava",T.LAVA,"Lava","Dauerhaft unbetretbar und gefährlich.",NEW_V["Lava"],{"Moveable":0.0,"BlocksVision":0.0,"TurnStartDamage":180.0,"OnEnterDamage":180.0,"FireDurationInitiative":0.0}),
 T.BASALT: terrain("DA_TerrainBasalt",T.BASALT,"Basalt","Unbetretbar und sichtblockierend.",NEW_V["Basalt"],{"Moveable":0.0,"BlocksVision":1.0}),
 T.STONE_FLOOR: terrain("DA_TerrainStoneFloor",T.STONE_FLOOR,"Steinboden","Neutraler Boden alter Ruinen.",NEW_V["StoneFloor"],{"Moveable":1.0,"MoveCostMultiplier":1.0}),
 T.BRIDGE: terrain("DA_TerrainBridge",T.BRIDGE,"Brücke","Schneller Übergang über Wasser.",NEW_V["Bridge"],{"Moveable":1.0,"MoveCostMultiplier":0.8}),
 T.GATE: terrain("DA_TerrainGate",T.GATE,"Tor","Begehbare Öffnung in einer Mauer.",NEW_V["Gate"],{"Moveable":1.0,"MoveCostMultiplier":1.0})}
mountain = terrain("DA_TerrainMountain",T.MOUNTAIN,"Mountain","Nicht betretbar; blockiert Sicht.",NEW_V["Mountain"],{"Moveable":0.0,"BlocksVision":1.0})
wall = terrain("DA_TerrainWall",T.WALL,"Mauer","Zerstörbare, sichtblockierende Mauer.",NEW_V["Wall"],{"Moveable":0.0,"BlocksVision":1.0,"MaxHealth":450.0})

def layer(layer_id, terrain_type, pattern, coverage, patch_min=1, patch_max=1, source="", depth_min=0, depth_max=0,
          path_starts=None, path_ends=None, path_minimum_length=4, minimum_crossing_gap=0,
          maximum_uncrossed_path_fields=0, require_walkable_opposite_sides=False):
    x=unreal.RoundTerrainRegionLayer(); x.set_editor_property("layer_id",layer_id); x.set_editor_property("terrain_type",terrain_type)
    x.set_editor_property("pattern",pattern); x.set_editor_property("coverage",coverage); x.set_editor_property("patch_size_min",patch_min)
    x.set_editor_property("patch_size_max",patch_max); x.set_editor_property("source_layer_id",source); x.set_editor_property("depth_min",depth_min); x.set_editor_property("depth_max",depth_max)
    x.set_editor_property("path_start_region_ids", path_starts or []); x.set_editor_property("path_end_region_ids", path_ends or [])
    x.set_editor_property("path_minimum_length",path_minimum_length); x.set_editor_property("minimum_crossing_gap",minimum_crossing_gap)
    x.set_editor_property("maximum_uncrossed_path_fields",maximum_uncrossed_path_fields)
    x.set_editor_property("require_walkable_opposite_sides",require_walkable_opposite_sides); return x
P=unreal.RoundTerrainLayerPattern; RP=unreal.RoundTerrainRegionPlacement
ST=unreal.RoundTerrainStructureTemplate
def structure(template,terrain_type,weight,coverage,cluster_min=1,cluster_max=1,passage_width=1):
    x=unreal.RoundTerrainRegionStructure(); x.set_editor_property("template",template); x.set_editor_property("terrain_type",terrain_type)
    x.set_editor_property("weight",weight); x.set_editor_property("coverage",coverage); x.set_editor_property("cluster_count_min",cluster_min)
    x.set_editor_property("cluster_count_max",cluster_max); x.set_editor_property("passage_width",passage_width); return x
def region(name,rid,display,color,weight,base,layers,size_min,size_max,placement=RP.ANY,edge_distance=2,minimum=0,maximum=0,can_generate=True,structures=None,global_feature=False):
    a=create_or_load(name,REGION_ROOT,unreal.RoundTerrainRegionData); a.set_editor_property("region_id",rid); a.set_editor_property("display_name",unreal.Text(display))
    a.set_editor_property("debug_color",color); a.set_editor_property("base_weight",weight); a.set_editor_property("base_terrain_type",base); a.set_editor_property("layers",layers)
    a.set_editor_property("region_size_min",size_min); a.set_editor_property("region_size_max",size_max); a.set_editor_property("placement",placement)
    a.set_editor_property("edge_distance",edge_distance); a.set_editor_property("minimum_occurrences",minimum); a.set_editor_property("maximum_occurrences",maximum); a.set_editor_property("can_generate",can_generate)
    a.set_editor_property("global_feature",global_feature)
    a.set_editor_property("structures",structures or [])
    unreal.EditorAssetLibrary.save_loaded_asset(a); return a

R={}
R["Start"]=region("DA_TerrainRegion_Start","Start","Startgebiet",unreal.LinearColor(0.15,0.65,1,0.58),0,T.GROUND,[],1,1,can_generate=False)
R["Plains"]=region("DA_TerrainRegion_Plains","Plains","Ebene",unreal.LinearColor(0.92,0.78,0.2,0.55),1.35,T.MEADOW,[layer("Road",T.STREET,P.PATH,0.08),layer("Scrub",T.RUBBLE,P.SCATTER,0.05)],85,135,minimum=1,structures=[
    structure(ST.OPEN,T.DENSE_FOREST,0.25,0.06), structure(ST.SCATTERED,T.DENSE_FOREST,0.45,0.16,2,4,2), structure(ST.SPLIT,T.DENSE_FOREST,0.30,0.16,1,1,2)])
R["Forest"]=region("DA_TerrainRegion_Forest","Forest","Waldgebiet",unreal.LinearColor(0.12,0.72,0.24,0.55),1.15,T.FOREST,[layer("Groves",T.GROVE,P.CLUSTER,0.12,3,7),layer("Thorns",T.THORNS,P.CLUSTER,0.07,2,5)],80,125,minimum=1,structures=[
    structure(ST.LANDMARK,T.DENSE_FOREST,0.15,0.10,1,1,2), structure(ST.SCATTERED,T.DENSE_FOREST,0.35,0.24,2,4,2),
    structure(ST.CORRIDOR,T.DENSE_FOREST,0.30,0.24,1,1,2), structure(ST.RING,T.DENSE_FOREST,0.20,0.22,1,1,2)])
R["Rocky"]=region("DA_TerrainRegion_Rocky","Rocky","Felsland",unreal.LinearColor(0.9,0.38,0.12,0.55),0.8,T.RUBBLE,[layer("Rock",T.ROCKS,P.SCATTER,0.1),layer("Trail",T.STREET,P.PATH,0.08)],55,85,minimum=1,structures=[
    structure(ST.OPEN,T.ROCKS,0.15,0.08), structure(ST.SCATTERED,T.ROCKS,0.35,0.20,2,4,1),
    structure(ST.CHOKEPOINT,T.ROCKS,0.30,0.22,1,1,2), structure(ST.CORRIDOR,T.ROCKS,0.20,0.24,1,1,2)])
R["Mountain"]=region("DA_TerrainRegion_Mountain","Mountain","Gebirge",unreal.LinearColor(0.62,0.25,0.88,0.6),0.24,T.MOUNTAIN,[],12,22,maximum=2)
R["Lake"]=region("DA_TerrainRegion_Lake","Lake","See",unreal.LinearColor(0.08,0.38,0.82,0.58),0.38,T.DEEP_WATER,[layer("LakeShore",T.SHALLOW_WATER,P.REGION_BORDER,1,depth_min=0,depth_max=0)],22,38,maximum=2,structures=[
    structure(ST.LANDMARK,T.MEADOW,0.65,0.10), structure(ST.SCATTERED,T.MEADOW,0.35,0.14,2,3)])
R["Coast"]=region("DA_TerrainRegion_Coast","Coast","Küste",unreal.LinearColor(0.1,0.68,0.85,0.58),0.7,T.SAND,[layer("Shallows",T.SHALLOW_WATER,P.MAP_EDGE_BAND,1,depth_min=1,depth_max=2),layer("Deep",T.DEEP_WATER,P.MAP_EDGE_BAND,1,depth_min=0,depth_max=0)],45,75,RP.EDGE_ONLY,2,0,1,structures=[
    structure(ST.SCATTERED,T.DUNE,0.65,0.14,2,4,2), structure(ST.LANDMARK,T.ROCKS,0.35,0.08,1,1,2)])
R["River"]=region("DA_TerrainRegion_River","River","Fluss",unreal.LinearColor(0.08,0.42,0.95,0.58),0.55,T.GROUND,[layer("RiverCore",T.DEEP_WATER,P.PATH,0.16,path_starts=["Mountain","Lake"],path_ends=["Lake","Coast"],path_minimum_length=8),layer("RiverBank",T.SHALLOW_WATER,P.ADJACENT_TO_LAYER,0.3,source="RiverCore"),layer("Bridge",T.BRIDGE,P.CROSSING,0.025,source="RiverCore",minimum_crossing_gap=2,maximum_uncrossed_path_fields=6,require_walkable_opposite_sides=True)],1,1,can_generate=False,global_feature=True)
R["Swamp"]=region("DA_TerrainRegion_Swamp","Swamp","Sumpf",unreal.LinearColor(0.28,0.48,0.12,0.58),0.42,T.MUD,[layer("Pools",T.SHALLOW_WATER,P.CLUSTER,0.12,3,7),layer("Reeds",T.REEDS,P.ADJACENT_TO_LAYER,0.25,source="Pools"),layer("Islands",T.MEADOW,P.SCATTER,0.08)],45,70,maximum=1,structures=[
    structure(ST.SCATTERED,T.DEEP_WATER,0.45,0.18,2,4,2), structure(ST.SPLIT,T.DEEP_WATER,0.30,0.18,1,1,2), structure(ST.CORRIDOR,T.REEDS,0.25,0.20,1,1,2)])
R["Desert"]=region("DA_TerrainRegion_Desert","Desert","Wüste",unreal.LinearColor(0.95,0.6,0.12,0.58),0.4,T.SAND,[layer("Dunes",T.DUNE,P.SCATTER,0.08),layer("Oasis",T.OASIS,P.CLUSTER,0.045,2,4)],55,90,RP.INTERIOR_ONLY,2,0,1,structures=[
    structure(ST.CORRIDOR,T.DUNE,0.35,0.20,1,1,2), structure(ST.RING,T.DUNE,0.25,0.18,1,1,2), structure(ST.SCATTERED,T.ROCKS,0.40,0.14,2,4,2)])
R["Volcanic"]=region("DA_TerrainRegion_Volcanic","Volcanic","Vulkanland",unreal.LinearColor(0.88,0.1,0.05,0.6),0.16,T.ASH,[layer("Lava",T.LAVA,P.PATH,0.08),layer("Basalt",T.BASALT,P.ADJACENT_TO_LAYER,0.14,source="Lava")],28,48,RP.INTERIOR_ONLY,3,0,1,structures=[
    structure(ST.SPLIT,T.LAVA,0.35,0.16,1,1,2), structure(ST.RING,T.BASALT,0.35,0.20,1,1,2), structure(ST.SCATTERED,T.BASALT,0.30,0.18,2,3,2)])
R["Ruins"]=region("DA_TerrainRegion_Ruins","Ruins","Ruinen",unreal.LinearColor(0.58,0.58,0.62,0.58),0.25,T.STONE_FLOOR,[layer("Walls",T.WALL,P.PATH,0.07),layer("Gates",T.GATE,P.CROSSING,0.035,source="Walls",minimum_crossing_gap=2,require_walkable_opposite_sides=True),layer("Debris",T.RUBBLE,P.SCATTER,0.08)],35,60,maximum=1,structures=[
    structure(ST.RING,T.WALL,0.40,0.22,1,1,2), structure(ST.CORRIDOR,T.WALL,0.30,0.20,1,1,2), structure(ST.CHOKEPOINT,T.WALL,0.30,0.18,1,1,2)])
R["Hedgeland"]=region("DA_TerrainRegion_Hedgeland","Hedgeland","Heckenland",unreal.LinearColor(0.36,0.72,0.12,0.58),0.72,T.MEADOW,[layer("TallGrass",T.TALL_GRASS,P.CLUSTER,0.24,4,10),layer("Paths",T.STREET,P.PATH,0.05)],65,105,maximum=2,structures=[
    structure(ST.SCATTERED,T.HEDGE,0.35,0.17,2,4,2), structure(ST.SPLIT,T.HEDGE,0.30,0.16,1,1,2),
    structure(ST.RING,T.HEDGE,0.20,0.16,1,1,2), structure(ST.OPEN,T.TALL_GRASS,0.15,0.10,1,1,2)])
R["Canyon"]=region("DA_TerrainRegion_Canyon","Canyon","Schlucht",unreal.LinearColor(0.68,0.28,0.08,0.60),0.40,T.CANYON_FLOOR,[layer("Rubble",T.RUBBLE,P.SCATTER,0.08)],45,75,RP.INTERIOR_ONLY,2,0,1,structures=[
    structure(ST.CORRIDOR,T.CLIFF,0.45,0.28,1,1,2), structure(ST.CHOKEPOINT,T.CLIFF,0.35,0.24,1,1,2),
    structure(ST.SPLIT,T.CLIFF,0.20,0.22,1,1,2)])
R["Mistmoor"]=region("DA_TerrainRegion_Mistmoor","Mistmoor","Nebelmoor",unreal.LinearColor(0.34,0.50,0.48,0.58),0.34,T.BOG,[layer("FogBanks",T.FOG,P.CLUSTER,0.32,5,12),layer("Reeds",T.REEDS,P.CLUSTER,0.10,3,7)],45,75,maximum=1,structures=[
    structure(ST.SCATTERED,T.DEEP_WATER,0.40,0.14,2,4,2), structure(ST.CORRIDOR,T.FOG,0.35,0.22,1,1,2),
    structure(ST.RING,T.REEDS,0.25,0.18,1,1,2)])
R["Highland"]=region("DA_TerrainRegion_Highland","Highland","Hochland",unreal.LinearColor(0.52,0.58,0.22,0.58),0.48,T.HIGHLAND,[layer("Rock",T.ROCKS,P.SCATTER,0.06)],50,85,maximum=2,structures=[
    structure(ST.CORRIDOR,T.RIDGE,0.40,0.20,1,1,2), structure(ST.RING,T.RIDGE,0.30,0.18,1,1,2),
    structure(ST.SCATTERED,T.RIDGE,0.30,0.16,2,3,2)])

def adjacency(a,b,weight):
    x=unreal.RoundTerrainRegionAdjacency(); x.set_editor_property("region_a",a); x.set_editor_property("region_b",b); x.set_editor_property("weight",weight); return x
ids=list(R.keys()); weights={tuple(sorted((a,b))):1.0 for a in ids for b in ids}
def w(a,b,v): weights[tuple(sorted((a,b)))]=v
w("Start","Coast",0); w("Start","Volcanic",0); w("Start","Mountain",0); w("Coast","Swamp",1.5); w("Coast","River",1.8); w("Coast","Desert",1.2); w("Coast","Volcanic",0)
w("Start","Lake",0); w("Lake","River",2.5); w("Lake","Mountain",1.2); w("Lake","Forest",1.1); w("Lake","Swamp",1.4); w("Lake","Volcanic",0); w("Lake","Coast",0.6)
w("River","Swamp",2); w("River","Desert",0.45); w("River","Volcanic",0.1); w("River","Ruins",1.2); w("Swamp","Desert",0); w("Swamp","Volcanic",0); w("Swamp","Forest",1.6)
w("Desert","Forest",0.15); w("Desert","Volcanic",0.65); w("Desert","Rocky",1.3); w("Volcanic","Rocky",1.8); w("Volcanic","Mountain",1.7); w("Volcanic","Forest",0)
w("Ruins","Plains",1.4); w("Ruins","Forest",1.1); w("Ruins","Rocky",1.1)
w("Hedgeland","Plains",2.0); w("Hedgeland","Forest",1.5); w("Hedgeland","Swamp",0.8); w("Hedgeland","Volcanic",0); w("Hedgeland","Mountain",0.3)
w("Canyon","Rocky",2.2); w("Canyon","Mountain",1.8); w("Canyon","Desert",1.5); w("Canyon","Swamp",0); w("Canyon","Mistmoor",0); w("Canyon","Coast",0.2)
w("Mistmoor","Swamp",2.2); w("Mistmoor","Forest",1.5); w("Mistmoor","Lake",1.6); w("Mistmoor","Desert",0); w("Mistmoor","Volcanic",0); w("Mistmoor","Plains",0.5)
w("Highland","Plains",1.4); w("Highland","Rocky",1.8); w("Highland","Mountain",2.0); w("Highland","Swamp",0.2); w("Highland","Mistmoor",0.3); w("Highland","Coast",0.4)
entries=[adjacency(a,b,weights[tuple(sorted((a,b)))]) for i,a in enumerate(ids) for b in ids[i:]]
registry=create_or_load("DA_TerrainRegionRegistry",REGION_ROOT,unreal.RoundTerrainRegionRegistryData); registry.set_editor_property("regions",R); registry.set_editor_property("adjacencies",entries)
registry.set_editor_property("start_region_id","Start"); registry.set_editor_property("start_region_radius",3); registry.set_editor_property("off_map_growth_weight",0.35)
registry.set_editor_property("ensure_walkable_connectivity",True); registry.set_editor_property("ensure_start_positions_connected",True); registry.set_editor_property("connectivity_pass_terrain_type",T.RUBBLE); unreal.EditorAssetLibrary.save_loaded_asset(registry)
terrain_registry=unreal.EditorAssetLibrary.load_asset(f"{TERRAIN_ROOT}/DA_Terrains"); terrain_map=dict(terrain_registry.get_editor_property("terrain_data_by_type")); terrain_map.update(new_terrains)
terrain_map[T.MOUNTAIN]=mountain; terrain_map[T.WALL]=wall; terrain_registry.set_editor_property("terrain_data_by_type",terrain_map); unreal.EditorAssetLibrary.save_loaded_asset(terrain_registry)
game_registry=unreal.EditorAssetLibrary.load_asset("/Game/TopDown/DA_RoundGameDataRegistry"); game_registry.set_editor_property("terrain_region_registry",registry); unreal.EditorAssetLibrary.save_loaded_asset(game_registry)
unreal.log("Created terrain fields and distinct region package.")
