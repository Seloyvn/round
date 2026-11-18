import os

import unreal


TARGETS = [
    "Character_2", "Character_3", "Character_4", "Character_5",
    "Bladecaller", "Chronomancer", "Cleric", "Dragoon", "Druid",
    "Engineer", "Gambler", "Geomancer", "Paladin",
]


def main():
    project_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    tasks = []
    for character_id in TARGETS:
        source = os.path.join(
            project_dir, "Content", "TopDown", "Characters", character_id,
            "Model", f"{character_id}_Placeholder.fbx"
        )
        if not os.path.isfile(source):
            raise RuntimeError(f"missing FBX: {source}")
        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = f"/Game/TopDown/Characters/{character_id}/Model"
        task.destination_name = f"{character_id}_Placeholder"
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = True
        task.save = True
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_as_skeletal = False
        options.import_materials = True
        options.import_textures = False
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.generate_lightmap_u_vs = True
        task.options = options
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        if not task.imported_object_paths:
            unreal.log_error(f"PLACEHOLDER_IMPORT failed {task.filename}")
        else:
            unreal.log(f"PLACEHOLDER_IMPORT {task.imported_object_paths}")


if __name__ == "__main__":
    main()
