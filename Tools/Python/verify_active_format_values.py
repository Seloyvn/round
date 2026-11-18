import unreal


table = unreal.load_asset("/Game/TopDown/DT_FormatNative")
if not isinstance(table, unreal.DataTable):
    raise RuntimeError("Could not load DT_FormatNative")

field_size = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "FieldSize")
terrain = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Terrain")
format_value = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Format")
name_value = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Name")
if format_value != ["Format5x16"] or name_value != ["5-16"] or field_size != ["15"] or terrain != ["150"]:
    raise RuntimeError(
        f"Unexpected values: Format={format_value}, Name={name_value}, "
        f"FieldSize={field_size}, Terrain={terrain}"
    )

unreal.log(
    f"Disk verification passed: Format={format_value}, Name={name_value}, "
    f"FieldSize={field_size}, Terrain={terrain}"
)
