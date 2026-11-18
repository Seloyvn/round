import math
import os
import sys

import bpy
from mathutils import Vector


CHARACTERS = {
    "Character_2": {"label": "Marksman", "colors": ((0.12, 0.22, 0.32, 1), (0.55, 0.72, 0.82, 1), (0.20, 0.12, 0.08, 1)), "prop": "rifle", "build": "slim"},
    "Character_3": {"label": "Assassin", "colors": ((0.10, 0.07, 0.16, 1), (0.42, 0.18, 0.55, 1), (0.12, 0.12, 0.14, 1)), "prop": "daggers", "build": "slim"},
    "Character_4": {"label": "Mage", "colors": ((0.10, 0.18, 0.35, 1), (0.20, 0.65, 0.95, 1), (0.75, 0.35, 0.12, 1)), "prop": "staff", "build": "slim"},
    "Character_5": {"label": "Bruiser", "colors": ((0.32, 0.06, 0.06, 1), (0.70, 0.12, 0.10, 1), (0.22, 0.20, 0.18, 1)), "prop": "axe", "build": "heavy"},
    "Bladecaller": {"label": "Bladecaller", "colors": ((0.10, 0.20, 0.23, 1), (0.20, 0.75, 0.75, 1), (0.62, 0.72, 0.78, 1)), "prop": "floating_blades", "build": "medium"},
    "Chronomancer": {"label": "Chronomancer", "colors": ((0.20, 0.10, 0.32, 1), (0.80, 0.55, 0.12, 1), (0.25, 0.75, 0.85, 1)), "prop": "time_staff", "build": "slim"},
    "Cleric": {"label": "Cleric", "colors": ((0.72, 0.66, 0.48, 1), (0.95, 0.88, 0.58, 1), (0.35, 0.55, 0.82, 1)), "prop": "mace_book", "build": "medium"},
    "Dragoon": {"label": "Dragoon", "colors": ((0.12, 0.18, 0.28, 1), (0.28, 0.52, 0.78, 1), (0.66, 0.70, 0.75, 1)), "prop": "spear", "build": "medium"},
    "Druid": {"label": "Druid", "colors": ((0.12, 0.28, 0.12, 1), (0.38, 0.62, 0.18, 1), (0.32, 0.17, 0.08, 1)), "prop": "antler_staff", "build": "heavy"},
    "Engineer": {"label": "Engineer", "colors": ((0.20, 0.22, 0.24, 1), (0.90, 0.48, 0.08, 1), (0.48, 0.55, 0.58, 1)), "prop": "wrench_pack", "build": "medium"},
    "Gambler": {"label": "Gambler", "colors": ((0.22, 0.04, 0.08, 1), (0.82, 0.12, 0.20, 1), (0.92, 0.82, 0.62, 1)), "prop": "cards", "build": "slim"},
    "Geomancer": {"label": "Geomancer", "colors": ((0.28, 0.18, 0.08, 1), (0.72, 0.48, 0.16, 1), (0.30, 0.55, 0.32, 1)), "prop": "stone_gauntlets", "build": "heavy"},
    "Paladin": {"label": "Paladin", "colors": ((0.62, 0.65, 0.70, 1), (0.92, 0.72, 0.18, 1), (0.18, 0.32, 0.58, 1)), "prop": "shield_hammer", "build": "heavy"},
}


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in (bpy.data.meshes, bpy.data.curves, bpy.data.materials, bpy.data.cameras, bpy.data.lights):
        pass


def material(name, color, metallic=0.0, roughness=0.55):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat


def finish(obj, mat, name):
    obj.name = name
    obj.data.materials.append(mat)
    bpy.ops.object.shade_smooth()
    return obj


def cube(name, loc, scale, mat, rot=(0, 0, 0), bevel=0.06):
    bpy.ops.mesh.primitive_cube_add(location=loc, rotation=rot)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new("SoftEdges", "BEVEL")
        mod.width = bevel
        mod.segments = 2
    return finish(obj, mat, name)


def cylinder(name, loc, radius, depth, mat, rot=(0, 0, 0), vertices=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rot)
    return finish(bpy.context.object, mat, name)


def sphere(name, loc, scale, mat, segments=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=8, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, mat, name)


def cone(name, loc, radius1, radius2, depth, mat, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cone_add(vertices=12, radius1=radius1, radius2=radius2, depth=depth, location=loc, rotation=rot)
    return finish(bpy.context.object, mat, name)


def blade(name, loc, length, mat, rot=(0, 0, 0)):
    return cube(name, loc, (0.055, 0.025, length / 2), mat, rot, 0.025)


def base_body(build, primary, accent, skin):
    widths = {"slim": 0.50, "medium": 0.62, "heavy": 0.76}
    w = widths[build]
    parts = []
    parts += [cylinder("Torso", (0, 0, 1.55), w, 0.88, primary, vertices=12)]
    parts += [sphere("Head", (0, 0, 2.30), (0.34, 0.31, 0.38), skin)]
    parts += [cylinder("Neck", (0, 0, 2.00), 0.18, 0.22, accent, vertices=10)]
    shoulder_x = w + 0.22
    for side, sign in (("L", -1), ("R", 1)):
        parts += [sphere(f"Shoulder_{side}", (sign * shoulder_x, 0, 1.78), (0.27, 0.29, 0.27), accent)]
        parts += [cylinder(f"Arm_{side}", (sign * (shoulder_x + 0.05), 0, 1.25), 0.17, 0.74, primary, vertices=10)]
        parts += [sphere(f"Hand_{side}", (sign * (shoulder_x + 0.05), 0, 0.82), (0.18, 0.17, 0.18), skin)]
        parts += [cylinder(f"Leg_{side}", (sign * 0.25, 0, 0.55), 0.22 if build == "heavy" else 0.18, 0.85, primary, vertices=10)]
        parts += [cube(f"Boot_{side}", (sign * 0.25, -0.06, 0.12), (0.25, 0.32, 0.14), accent)]
    parts += [cube("Belt", (0, 0, 1.08), (w + 0.05, 0.34, 0.10), accent)]
    return parts


def add_prop(kind, parts, primary, accent, tertiary):
    if kind == "rifle":
        parts += [cube("RifleStock", (0.72, -0.18, 1.20), (0.12, 0.12, 0.45), tertiary, (0, math.radians(18), 0))]
        parts += [cylinder("RifleBarrel", (0.86, -0.18, 1.72), 0.055, 0.85, accent, (0, math.radians(15), 0))]
        parts += [cylinder("Scope", (0.75, -0.18, 1.63), 0.07, 0.28, accent, (0, math.radians(15), 0))]
    elif kind == "daggers":
        parts += [blade("Dagger_L", (-0.82, -0.03, 0.72), 0.65, accent, (0, math.radians(-28), 0))]
        parts += [blade("Dagger_R", (0.82, -0.03, 0.72), 0.65, accent, (0, math.radians(28), 0))]
        parts += [cone("Hood", (0, 0.03, 2.48), 0.42, 0.16, 0.48, primary)]
    elif kind in ("staff", "time_staff", "antler_staff"):
        parts += [cylinder("Staff", (0.92, 0, 1.15), 0.055, 2.40, tertiary)]
        parts += [sphere("Focus", (0.92, 0, 2.40), (0.24, 0.24, 0.24), accent)]
        if kind == "time_staff":
            bpy.ops.mesh.primitive_torus_add(major_radius=0.34, minor_radius=0.045, major_segments=20, minor_segments=6, location=(0.92, 0, 2.40), rotation=(math.pi / 2, 0, 0))
            parts += [finish(bpy.context.object, accent, "TimeRing")]
        if kind == "antler_staff":
            parts += [cone("Antler_L", (0.76, 0, 2.66), 0.07, 0.015, 0.55, tertiary, (0, math.radians(-30), 0))]
            parts += [cone("Antler_R", (1.08, 0, 2.66), 0.07, 0.015, 0.55, tertiary, (0, math.radians(30), 0))]
    elif kind == "axe":
        parts += [cylinder("AxeHandle", (0.88, 0, 1.18), 0.06, 1.70, tertiary)]
        parts += [cube("AxeHead", (0.88, 0, 2.04), (0.38, 0.09, 0.22), accent, (0, 0, math.radians(12)))]
    elif kind == "floating_blades":
        for i, (x, z, angle) in enumerate(((-1.15, 1.15, -24), (1.18, 1.42, 28), (-0.85, 2.45, 12))):
            parts += [blade(f"FloatingBlade_{i}", (x, 0.12, z), 0.85, accent, (0, math.radians(angle), 0))]
    elif kind == "mace_book":
        parts += [cylinder("MaceHandle", (-0.86, 0, 1.05), 0.055, 1.20, tertiary)]
        parts += [sphere("MaceHead", (-0.86, 0, 1.70), (0.22, 0.22, 0.22), accent)]
        parts += [cube("Book", (0.80, -0.05, 0.92), (0.24, 0.09, 0.31), primary)]
        parts += [cube("BookCross", (0.80, -0.15, 0.92), (0.04, 0.02, 0.18), accent, bevel=0.01)]
    elif kind == "spear":
        parts += [cylinder("SpearShaft", (0.88, 0, 1.38), 0.045, 2.85, tertiary)]
        parts += [cone("SpearHead", (0.88, 0, 2.94), 0.17, 0.0, 0.55, accent)]
        parts += [cone("HelmetCrest", (0, 0, 2.78), 0.08, 0.02, 0.62, accent, (0, math.radians(-12), 0))]
    elif kind == "wrench_pack":
        parts += [cube("Backpack", (0, 0.38, 1.62), (0.48, 0.22, 0.58), tertiary)]
        parts += [cylinder("Wrench", (0.86, 0, 1.08), 0.07, 1.15, accent, (0, math.radians(16), 0))]
        parts += [sphere("Goggle_L", (-0.16, -0.28, 2.34), (0.12, 0.05, 0.12), accent)]
        parts += [sphere("Goggle_R", (0.16, -0.28, 2.34), (0.12, 0.05, 0.12), accent)]
    elif kind == "cards":
        for i in range(3):
            parts += [cube(f"Card_{i}", (0.70 + i * 0.09, -0.13, 0.86 + i * 0.04), (0.10, 0.015, 0.15), accent, (0, math.radians(i * 8 - 8), 0), 0.01)]
        parts += [cone("Hat", (0, 0, 2.64), 0.42, 0.22, 0.34, primary)]
        parts += [cube("HatBrim", (0, 0, 2.49), (0.52, 0.40, 0.05), accent)]
    elif kind == "stone_gauntlets":
        parts += [cube("StoneGauntlet_L", (-0.92, 0, 0.84), (0.30, 0.34, 0.32), accent)]
        parts += [cube("StoneGauntlet_R", (0.92, 0, 0.84), (0.30, 0.34, 0.32), accent)]
        parts += [cube("StonePauldrons", (0, 0.05, 1.88), (1.02, 0.42, 0.18), tertiary)]
    elif kind == "shield_hammer":
        parts += [cylinder("Shield", (-0.92, -0.18, 1.18), 0.52, 0.14, primary, (math.pi / 2, 0, 0), vertices=12)]
        parts += [cube("ShieldCrossV", (-0.92, -0.28, 1.18), (0.07, 0.03, 0.34), accent)]
        parts += [cube("ShieldCrossH", (-0.92, -0.28, 1.18), (0.28, 0.03, 0.07), accent)]
        parts += [cylinder("HammerHandle", (0.90, 0, 1.15), 0.06, 1.45, tertiary)]
        parts += [cube("HammerHead", (0.90, 0, 1.92), (0.34, 0.20, 0.20), accent)]


def join_export(parts, name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    obj.scale = (60, 60, 60)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def setup_preview(scale=60.0):
    bpy.ops.object.camera_add(location=(3.8 * scale, -5.6 * scale, 3.5 * scale), rotation=(math.radians(69), 0, math.radians(34)))
    camera = bpy.context.object
    camera.data.clip_end = 5000.0
    bpy.context.scene.camera = camera
    direction = Vector((0, 0, 1.35 * scale)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    bpy.ops.object.light_add(type="AREA", location=(-3.5 * scale, -4.5 * scale, 7.0 * scale))
    bpy.context.object.data.energy = 120000
    bpy.context.object.data.shape = "DISK"
    bpy.context.object.data.size = 5.0 * scale
    bpy.ops.object.light_add(type="AREA", location=(4.0 * scale, 1.5 * scale, 4.0 * scale))
    bpy.context.object.data.energy = 80000
    bpy.context.object.data.color = (0.45, 0.65, 1.0)
    bpy.context.object.data.size = 4.0 * scale
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 768
    scene.render.resolution_y = 768
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 2.0


def build_character(character_id, config, project_root):
    reset_scene()
    primary = material("M_Primary", config["colors"][0], metallic=0.25)
    accent = material("M_Accent", config["colors"][1], metallic=0.6, roughness=0.32)
    tertiary = material("M_Tertiary", config["colors"][2], metallic=0.15)
    skin = material("M_Skin", (0.62, 0.42, 0.30, 1), roughness=0.75)
    parts = base_body(config["build"], primary, accent, skin)
    add_prop(config["prop"], parts, primary, accent, tertiary)
    export = join_export(parts, f"{character_id}_Placeholder_Export")
    output_dir = os.path.join(project_root, "Content", "TopDown", "Characters", character_id, "Model")
    os.makedirs(output_dir, exist_ok=True)
    blend_path = os.path.join(output_dir, f"{character_id}_Placeholder.blend")
    fbx_path = os.path.join(output_dir, f"{character_id}_Placeholder.fbx")
    glb_path = os.path.join(output_dir, f"{character_id}_Placeholder.glb")
    preview_path = os.path.join(output_dir, f"{character_id}_Placeholder_Preview.png")
    setup_preview()
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    bpy.ops.object.select_all(action="DESELECT")
    export.select_set(True)
    bpy.context.view_layer.objects.active = export
    bpy.ops.export_scene.fbx(filepath=fbx_path, use_selection=True, apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"}, add_leaf_bones=False)
    bpy.ops.export_scene.gltf(filepath=glb_path, use_selection=True, export_format="GLB", export_apply=True)
    bpy.context.scene.render.filepath = preview_path
    bpy.ops.render.render(write_still=True)
    print(f"generated {character_id}: {output_dir}")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    project_root = os.path.abspath(args[0]) if args else os.getcwd()
    requested = args[1:] or list(CHARACTERS)
    for character_id in requested:
        if character_id not in CHARACTERS:
            raise ValueError(f"unknown character: {character_id}")
        build_character(character_id, CHARACTERS[character_id], project_root)


if __name__ == "__main__":
    main()
