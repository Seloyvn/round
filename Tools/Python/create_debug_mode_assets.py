import unreal


REGISTRY_PATH = "/Game/TopDown/DA_RoundGameDataRegistry"
DEBUG_WIDGET_PATH = "/Game/TopDown/UI/WBP_DebugMode"
DEBUG_MATERIAL_PATH = "/Game/TopDown/UI/M_DebugRegionOverlay"


if not unreal.RoundWidgetBlueprintBuilder.rebuild_debug_widget_blueprint():
    raise RuntimeError("Debug widget blueprint could not be created")

registry = unreal.EditorAssetLibrary.load_asset(REGISTRY_PATH)
if not registry:
    raise RuntimeError("DA_RoundGameDataRegistry could not be loaded")

debug_widget_class = unreal.EditorAssetLibrary.load_blueprint_class(DEBUG_WIDGET_PATH)
if not debug_widget_class:
    raise RuntimeError("WBP_DebugMode generated class could not be loaded")

registry.set_editor_property("debug_widget_class", debug_widget_class)

debug_material = unreal.EditorAssetLibrary.load_asset(DEBUG_MATERIAL_PATH)
if not debug_material:
    debug_material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_DebugRegionOverlay", "/Game/TopDown/UI", unreal.Material, unreal.MaterialFactoryNew()
    )
    debug_material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    debug_material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        debug_material, unreal.MaterialExpressionVectorParameter, -320, 0
    )
    color.set_editor_property("parameter_name", "RegionColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.25, 0.55, 1.0, 0.55))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(color, "A", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(debug_material)
    unreal.EditorAssetLibrary.save_loaded_asset(debug_material)

registry.set_editor_property("debug_region_overlay_material", debug_material)
registry.set_editor_property("enable_debug_tools", True)
registry.set_editor_property("debug_damage_step", 100)
registry.set_editor_property("debug_initiative_step", 20)
unreal.EditorAssetLibrary.save_loaded_asset(registry)
unreal.EditorAssetLibrary.save_asset(DEBUG_WIDGET_PATH, only_if_is_dirty=False)
unreal.log("Round debug mode assets created and registered")
