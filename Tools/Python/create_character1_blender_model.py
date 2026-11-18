import math
import os
import sys

import bpy
from mathutils import Euler, Vector


def ensure_directory(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for block in bpy.data.meshes:
        bpy.data.meshes.remove(block)
    for block in bpy.data.materials:
        bpy.data.materials.remove(block)
    for block in bpy.data.curves:
        bpy.data.curves.remove(block)


def make_material(name: str, color: tuple[float, float, float, float], metallic: float, roughness: float):
    material = bpy.data.materials.new(name=name)
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
    return material


def assign_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    if obj.data.materials:
        obj.data.materials[0] = material
    else:
        obj.data.materials.append(material)


def add_cube(name: str, location: tuple[float, float, float], scale: tuple[float, float, float], material, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, material)
    return obj


def add_uv_sphere(name: str, location: tuple[float, float, float], scale: tuple[float, float, float], material, segments=24, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.shade_smooth()
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, material)
    return obj


def add_ico_sphere(name: str, location: tuple[float, float, float], scale: tuple[float, float, float], material, subdivisions=3):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.shade_smooth()
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, material)
    return obj


def add_cylinder(name: str, location: tuple[float, float, float], radius: float, depth: float, material, rotation=(0.0, 0.0, 0.0), vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.active_object
    obj.name = name
    assign_material(obj, material)
    return obj


def add_cone(name: str, location: tuple[float, float, float], radius1: float, radius2: float, depth: float, material, rotation=(0.0, 0.0, 0.0), vertices=20):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius1,
        radius2=radius2,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.active_object
    obj.name = name
    assign_material(obj, material)
    return obj


def add_torus(name: str, location: tuple[float, float, float], major_radius: float, minor_radius: float, material, rotation=(0.0, 0.0, 0.0), major_segments=20, minor_segments=10):
    bpy.ops.mesh.primitive_torus_add(
        major_segments=major_segments,
        minor_segments=minor_segments,
        major_radius=major_radius,
        minor_radius=minor_radius,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.active_object
    obj.name = name
    assign_material(obj, material)
    return obj


def bevel_object(obj: bpy.types.Object, width: float, segments: int = 2) -> None:
    modifier = obj.modifiers.new(name="Bevel", type="BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"


def shade_smooth_with_auto_smooth(obj: bpy.types.Object) -> None:
    if obj.type != "MESH":
        return
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.shade_smooth()
    if hasattr(obj.data, "use_auto_smooth"):
        obj.data.use_auto_smooth = True


def subdivide_object(obj: bpy.types.Object, levels: int = 1) -> None:
    modifier = obj.modifiers.new(name="Subdivision", type="SUBSURF")
    modifier.levels = levels
    modifier.render_levels = levels


def add_chain_link(name: str, location: tuple[float, float, float], material, rotation=(math.pi / 2.0, 0.0, 0.0)):
    return add_torus(
        name=name,
        location=location,
        major_radius=0.11,
        minor_radius=0.042,
        material=material,
        rotation=rotation,
        major_segments=20,
        minor_segments=12,
    )


def create_chain(start: Vector, end: Vector, material, root: bpy.types.Object) -> list[bpy.types.Object]:
    chain_objects = []
    direction = end - start
    distance = direction.length
    link_spacing = 0.18
    steps = max(3, int(distance / link_spacing))
    for index in range(steps + 1):
        alpha = index / steps
        position = start.lerp(end, alpha)
        rotation = (math.pi / 2.0, 0.0, 0.0 if index % 2 == 0 else math.pi / 2.0)
        link = add_chain_link(f"ChainLink_{index:02d}", tuple(position), material, rotation=rotation)
        link.parent = root
        chain_objects.append(link)
    return chain_objects


def create_chain_arc(points: list[Vector], material, root: bpy.types.Object, prefix: str) -> list[bpy.types.Object]:
    chain_objects = []
    link_index = 0
    for segment_index in range(len(points) - 1):
        start = points[segment_index]
        end = points[segment_index + 1]
        direction = end - start
        distance = direction.length
        link_spacing = 0.17
        steps = max(2, int(distance / link_spacing))
        for step in range(steps):
            alpha = step / steps
            position = start.lerp(end, alpha)
            rotation = (math.pi / 2.0, 0.0, math.radians(30.0) if link_index % 2 == 0 else math.radians(120.0))
            link = add_chain_link(f"{prefix}_{link_index:02d}", tuple(position), material, rotation=rotation)
            link.parent = root
            chain_objects.append(link)
            link_index += 1
    return chain_objects


def build_joined_export_mesh(export_name: str) -> bpy.types.Object:
    source_meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    for obj in bpy.data.objects:
        obj.select_set(False)

    for obj in source_meshes:
        obj.select_set(True)

    bpy.context.view_layer.objects.active = source_meshes[0]
    bpy.ops.object.duplicate(linked=False)
    duplicated_meshes = [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
    bpy.context.view_layer.objects.active = duplicated_meshes[0]
    bpy.ops.object.join()

    export_mesh = bpy.context.active_object
    export_mesh.name = export_name
    for modifier in list(export_mesh.modifiers):
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    export_mesh.scale = (60.0, 60.0, 60.0)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return export_mesh


def build_character(output_dir: str) -> None:
    clear_scene()

    bpy.context.scene.render.engine = "BLENDER_EEVEE_NEXT"
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0

    material_armor = make_material("M_ArmorSteel", (0.19, 0.23, 0.29, 1.0), 0.75, 0.35)
    material_dark = make_material("M_DarkMetal", (0.08, 0.09, 0.11, 1.0), 0.6, 0.5)
    material_accent = make_material("M_AccentBronze", (0.62, 0.45, 0.19, 1.0), 0.8, 0.28)
    material_cloth = make_material("M_ClothRed", (0.44, 0.06, 0.07, 1.0), 0.0, 0.88)
    material_skin = make_material("M_Skin", (0.72, 0.58, 0.48, 1.0), 0.0, 0.62)

    root = bpy.data.objects.new("Character1_ChainTank_Root", None)
    bpy.context.collection.objects.link(root)

    body_parts = [
        add_cone("Boot_L", (-0.25, 0.02, 0.23), 0.24, 0.16, 0.42, material_dark, vertices=18),
        add_cone("Boot_R", (0.25, 0.02, 0.23), 0.24, 0.16, 0.42, material_dark, vertices=18),
        add_cylinder("Shin_L", (-0.25, 0.02, 0.70), 0.15, 0.56, material_armor, vertices=18),
        add_cylinder("Shin_R", (0.25, 0.02, 0.70), 0.15, 0.56, material_armor, vertices=18),
        add_cube("KneeGuard_L", (-0.25, -0.10, 0.88), (0.14, 0.06, 0.11), material_accent, rotation=(math.radians(18.0), 0.0, 0.0)),
        add_cube("KneeGuard_R", (0.25, -0.10, 0.88), (0.14, 0.06, 0.11), material_accent, rotation=(math.radians(18.0), 0.0, 0.0)),
        add_cone("HipCore", (0.0, 0.03, 1.06), 0.42, 0.32, 0.34, material_cloth, vertices=20),
        add_cube("WaistPlate", (0.0, -0.14, 1.10), (0.46, 0.08, 0.10), material_dark, rotation=(math.radians(8.0), 0.0, 0.0)),
        add_cylinder("TorsoCore", (0.0, 0.06, 1.66), 0.46, 0.92, material_armor, vertices=22),
        add_cube("ChestPlate_Main", (0.0, -0.20, 1.74), (0.42, 0.07, 0.34), material_accent, rotation=(math.radians(8.0), 0.0, 0.0)),
        add_cube("ChestPlate_Upper", (0.0, -0.18, 1.97), (0.28, 0.06, 0.12), material_accent, rotation=(math.radians(10.0), 0.0, 0.0)),
        add_cube("ChestRib_L", (-0.35, -0.11, 1.66), (0.08, 0.08, 0.30), material_dark, rotation=(0.0, 0.0, math.radians(16.0))),
        add_cube("ChestRib_R", (0.35, -0.11, 1.66), (0.08, 0.08, 0.30), material_dark, rotation=(0.0, 0.0, math.radians(-16.0))),
        add_cylinder("NeckGuard", (0.0, 0.02, 2.18), 0.24, 0.18, material_dark, vertices=18),
        add_ico_sphere("Head", (0.0, 0.04, 2.38), (0.24, 0.26, 0.28), material_skin, subdivisions=3),
        add_cube("HelmetTop", (0.0, 0.06, 2.56), (0.36, 0.30, 0.18), material_armor),
        add_cube("HelmetBrow", (0.0, -0.18, 2.44), (0.28, 0.05, 0.08), material_dark, rotation=(math.radians(8.0), 0.0, 0.0)),
        add_cube("JawGuard", (0.0, -0.14, 2.28), (0.20, 0.06, 0.07), material_dark, rotation=(math.radians(-12.0), 0.0, 0.0)),
        add_cone("HelmetCrest", (0.0, 0.20, 2.70), 0.10, 0.04, 0.26, material_accent, rotation=(math.radians(-16.0), 0.0, 0.0), vertices=16),
        add_cube("Tabard_Main", (0.0, -0.14, 1.08), (0.22, 0.04, 0.42), material_cloth),
        add_cube("Tabard_Split_L", (-0.08, -0.10, 0.82), (0.08, 0.03, 0.18), material_cloth, rotation=(math.radians(6.0), 0.0, math.radians(6.0))),
        add_cube("Tabard_Split_R", (0.08, -0.10, 0.82), (0.08, 0.03, 0.18), material_cloth, rotation=(math.radians(6.0), 0.0, math.radians(-6.0))),
    ]

    arm_parts = [
        add_uv_sphere("Shoulder_L", (-0.72, 0.06, 1.94), (0.34, 0.30, 0.24), material_armor, segments=28, rings=16),
        add_uv_sphere("Shoulder_R", (0.72, 0.06, 1.94), (0.34, 0.30, 0.24), material_armor, segments=28, rings=16),
        add_cube("ShoulderTrim_L", (-0.92, -0.02, 1.90), (0.10, 0.08, 0.18), material_accent, rotation=(0.0, 0.0, math.radians(30.0))),
        add_cube("ShoulderTrim_R", (0.92, -0.02, 1.90), (0.10, 0.08, 0.18), material_accent, rotation=(0.0, 0.0, math.radians(-30.0))),
        add_cylinder("UpperArm_L", (-1.02, -0.02, 1.54), 0.13, 0.52, material_armor, rotation=(0.0, 0.0, math.radians(18.0)), vertices=16),
        add_cylinder("UpperArm_R", (1.02, -0.02, 1.54), 0.13, 0.52, material_armor, rotation=(0.0, 0.0, math.radians(-18.0)), vertices=16),
        add_cube("ElbowGuard_L", (-1.13, -0.08, 1.30), (0.10, 0.07, 0.10), material_accent, rotation=(math.radians(12.0), 0.0, math.radians(18.0))),
        add_cube("ElbowGuard_R", (1.13, -0.08, 1.30), (0.10, 0.07, 0.10), material_accent, rotation=(math.radians(12.0), 0.0, math.radians(-18.0))),
        add_cylinder("Forearm_L", (-1.30, -0.06, 1.00), 0.12, 0.54, material_dark, rotation=(0.0, 0.0, math.radians(28.0)), vertices=16),
        add_cylinder("Forearm_R", (1.30, -0.06, 1.00), 0.12, 0.54, material_dark, rotation=(0.0, 0.0, math.radians(-28.0)), vertices=16),
        add_torus("WristRing_L", (-1.42, -0.08, 0.79), 0.10, 0.03, material_accent, rotation=(math.pi / 2.0, 0.0, 0.0), major_segments=18, minor_segments=10),
        add_torus("WristRing_R", (1.42, -0.08, 0.79), 0.10, 0.03, material_accent, rotation=(math.pi / 2.0, 0.0, 0.0), major_segments=18, minor_segments=10),
        add_ico_sphere("Hand_L", (-1.48, -0.04, 0.74), (0.10, 0.09, 0.11), material_dark, subdivisions=2),
        add_ico_sphere("Hand_R", (1.48, -0.04, 0.74), (0.10, 0.09, 0.11), material_dark, subdivisions=2),
    ]

    extra_parts = [
        add_cube("BackPlate", (0.0, 0.26, 1.70), (0.38, 0.08, 0.44), material_dark),
        add_cube("BackPlate_Upper", (0.0, 0.22, 1.96), (0.26, 0.06, 0.16), material_dark),
        add_cube("HipGuard_L", (-0.38, -0.02, 1.04), (0.10, 0.18, 0.18), material_armor, rotation=(0.0, 0.0, math.radians(10.0))),
        add_cube("HipGuard_R", (0.38, -0.02, 1.04), (0.10, 0.18, 0.18), material_armor, rotation=(0.0, 0.0, math.radians(-10.0))),
        add_cone("ShoulderSpike_L", (-0.90, 0.22, 2.04), 0.05, 0.015, 0.26, material_accent, rotation=(math.radians(70.0), math.radians(10.0), math.radians(20.0)), vertices=12),
        add_cone("ShoulderSpike_R", (0.90, 0.22, 2.04), 0.05, 0.015, 0.26, material_accent, rotation=(math.radians(70.0), math.radians(-10.0), math.radians(-20.0)), vertices=12),
    ]

    for obj in body_parts + arm_parts + extra_parts:
        bevel_object(obj, 0.028, 3)
        shade_smooth_with_auto_smooth(obj)
        obj.parent = root

    for rounded_obj_name in [
        "Boot_L", "Boot_R", "Shin_L", "Shin_R", "HipCore", "TorsoCore",
        "NeckGuard", "HelmetTop", "UpperArm_L", "UpperArm_R", "Forearm_L", "Forearm_R"
    ]:
        rounded_obj = bpy.data.objects.get(rounded_obj_name)
        if rounded_obj:
            subdivide_object(rounded_obj, 1)

    left_chain_points = [
        Vector((-1.50, -0.02, 0.74)),
        Vector((-1.76, -0.20, 1.10)),
        Vector((-2.06, -0.30, 1.40)),
        Vector((-2.34, -0.20, 1.28)),
    ]
    right_chain_points = [
        Vector((1.50, -0.02, 0.74)),
        Vector((1.76, -0.20, 1.10)),
        Vector((2.06, -0.30, 1.40)),
        Vector((2.34, -0.20, 1.28)),
    ]
    left_chain = create_chain_arc(left_chain_points, material_accent, root, "ChainWhip_L")
    right_chain = create_chain_arc(right_chain_points, material_accent, root, "ChainWhip_R")
    for chain_link in left_chain + right_chain:
        shade_smooth_with_auto_smooth(chain_link)

    chain_weight_l = add_ico_sphere("ChainWeight_L", (-2.50, -0.12, 1.20), (0.14, 0.14, 0.14), material_accent, subdivisions=2)
    chain_weight_r = add_ico_sphere("ChainWeight_R", (2.50, -0.12, 1.20), (0.14, 0.14, 0.14), material_accent, subdivisions=2)
    chain_weight_l.parent = root
    chain_weight_r.parent = root

    for obj in [chain_weight_l, chain_weight_r]:
        bevel_object(obj, 0.02, 2)
        shade_smooth_with_auto_smooth(obj)

    bpy.ops.object.light_add(type="SUN", location=(0.0, 0.0, 5.0))
    sun = bpy.context.active_object
    sun.rotation_euler = Euler((math.radians(42.0), 0.0, math.radians(45.0)), "XYZ")
    sun.data.energy = 3.1

    bpy.ops.object.light_add(type="AREA", location=(2.0, -2.6, 3.8), rotation=(math.radians(62.0), 0.0, math.radians(35.0)))
    fill_light = bpy.context.active_object
    fill_light.data.energy = 1800
    fill_light.data.shape = "RECTANGLE"
    fill_light.data.size = 5.0
    fill_light.data.size_y = 5.0

    bpy.ops.object.camera_add(location=(6.8, -6.8, 5.6), rotation=(math.radians(58.0), 0.0, math.radians(45.0)))
    camera = bpy.context.active_object
    camera.name = "PreviewCamera"
    bpy.context.scene.camera = camera

    for obj in bpy.data.objects:
        obj.select_set(False)
    root.select_set(True)
    bpy.context.view_layer.objects.active = root

    export_mesh = build_joined_export_mesh("Character1_ChainTank_Export")

    blend_path = os.path.join(output_dir, "Character1_ChainTank_Blockout.blend")
    glb_path = os.path.join(output_dir, "Character1_ChainTank_Blockout.glb")
    fbx_path = os.path.join(output_dir, "Character1_ChainTank_Blockout.fbx")
    preview_path = os.path.join(output_dir, "Character1_ChainTank_Preview.png")

    bpy.ops.wm.save_as_mainfile(filepath=blend_path)

    for obj in bpy.data.objects:
        obj.select_set(obj == export_mesh)

    bpy.ops.export_scene.gltf(
        filepath=glb_path,
        use_selection=True,
        export_format="GLB",
        export_apply=True,
    )

    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        use_selection=True,
        apply_scale_options="FBX_SCALE_UNITS",
        bake_space_transform=False,
        object_types={"MESH", "EMPTY"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        axis_forward="-Z",
        axis_up="Y",
    )

    bpy.context.scene.render.filepath = preview_path
    bpy.context.scene.render.resolution_x = 1024
    bpy.context.scene.render.resolution_y = 1024
    bpy.context.scene.render.film_transparent = True
    bpy.ops.render.render(write_still=True)


def main() -> None:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    output_dir = argv[0] if argv else os.path.join(os.getcwd(), "Content", "TopDown", "Characters", "Character_1", "Model")
    ensure_directory(output_dir)
    build_character(output_dir)


if __name__ == "__main__":
    main()
