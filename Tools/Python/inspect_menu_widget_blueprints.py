import unreal


ASSETS = (
    "/Game/TopDown/UI/UI_MainMenu",
    "/Game/TopDown/Blueprints/UI_TeamsUserWidget",
    "/Game/TopDown/Blueprints/UI_SessionsUserWidget",
)


for asset_path in ASSETS:
    asset = unreal.load_asset(asset_path)
    unreal.log(f"WIDGET_ASSET {asset_path}: {asset} ({type(asset)})")
    if not asset:
        continue
    try:
        generated_class = asset.generated_class()
        unreal.log(f"WIDGET_GENERATED_CLASS {asset_path}: {generated_class}")
        default_object = unreal.get_default_object(generated_class)
        unreal.log(f"WIDGET_CDO {asset_path}: {default_object}")
        unreal.log(f"WIDGET_CDO_DIR {asset_path}: {','.join(sorted(dir(default_object)))}")
        tree = default_object.get_editor_property("widget_tree")
        unreal.log(f"WIDGET_CDO_TREE {asset_path}: {tree}")
        if tree:
            for widget in tree.get_all_widgets():
                unreal.log(
                    f"WIDGET_CDO_NODE {asset_path}: {widget.get_name()} "
                    f"class={widget.get_class().get_name()} slot={widget.slot}"
                )
    except Exception as exc:
        unreal.log_warning(f"WIDGET_CDO_FAILED {asset_path}: {exc}")
    unreal.log(f"WIDGET_DIR {asset_path}: {','.join(sorted(dir(asset)))}")
    for property_name in ("widget_tree", "generated_class", "parent_class"):
        try:
            value = asset.get_editor_property(property_name)
            unreal.log(f"WIDGET_PROPERTY {asset_path} {property_name}: {value}")
            if property_name == "widget_tree" and value:
                unreal.log(f"WIDGET_TREE_DIR {asset_path}: {','.join(sorted(dir(value)))}")
                try:
                    widgets = value.get_all_widgets()
                    for widget in widgets:
                        unreal.log(
                            f"WIDGET_NODE {asset_path}: {widget.get_name()} "
                            f"class={widget.get_class().get_name()} slot={widget.slot}"
                        )
                except Exception as exc:
                    unreal.log_warning(f"WIDGET_TREE_READ_FAILED {asset_path}: {exc}")
        except Exception as exc:
            unreal.log_warning(f"WIDGET_PROPERTY_FAILED {asset_path} {property_name}: {exc}")
