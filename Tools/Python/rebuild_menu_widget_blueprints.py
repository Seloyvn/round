import unreal


if not unreal.RoundWidgetBlueprintBuilder.rebuild_menu_widget_blueprints():
    raise RuntimeError("The Round menu widget blueprints could not be rebuilt")

unreal.log("ROUND_MENU_WIDGET_BLUEPRINTS_REBUILT")
