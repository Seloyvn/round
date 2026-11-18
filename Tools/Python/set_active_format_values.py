import csv
import io
import unreal


FORMAT_TABLE_PATH = "/Game/TopDown/DT_FormatNative"


def set_case_insensitive(row, field_name, value):
    for key in list(row.keys()):
        if key.casefold() == field_name.casefold():
            row[key] = value
            return
    raise RuntimeError(f"Field '{field_name}' was not found in {FORMAT_TABLE_PATH}")


table = unreal.load_asset(FORMAT_TABLE_PATH)
if not isinstance(table, unreal.DataTable):
    raise RuntimeError(f"Could not load DataTable {FORMAT_TABLE_PATH}")

row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
if len(row_names) != 1:
    raise RuntimeError(f"Expected exactly one format row, found {len(row_names)}")

columns = [
    "Format",
    "Name",
    "Characters",
    "Skillpoints",
    "FieldSize",
    "Terrain",
    "StartPosition",
    "SkillAmount",
    "TerrainWeights",
]
field_names = ["---"] + columns
row = {"---": str(row_names[0])}
for column in columns:
    values = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, column)
    if len(values) != 1:
        raise RuntimeError(f"Could not read column '{column}'")
    row[column] = values[0]

row["Format"] = "Format5x16"
row["Name"] = "5-16"
row["Characters"] = "5"
row["Skillpoints"] = "16"
row["FieldSize"] = "15"
row["Terrain"] = "150"

buffer = io.StringIO(newline="")
writer = csv.DictWriter(buffer, fieldnames=field_names, lineterminator="\n")
writer.writeheader()
writer.writerow(row)
if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, buffer.getvalue()):
    raise RuntimeError("Could not update format table from CSV")

unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
saved_field_size = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "FieldSize")
saved_terrain = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Terrain")
saved_format = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Format")
saved_name = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Name")
saved_characters = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Characters")
saved_skillpoints = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Skillpoints")
if (
    saved_format != ["Format5x16"]
    or saved_name != ["5-16"]
    or saved_characters != ["5"]
    or saved_skillpoints != ["16"]
    or saved_field_size != ["15"]
    or saved_terrain != ["150"]
):
    raise RuntimeError(
        f"Format verification failed: Format={saved_format}, Name={saved_name}, "
        f"Characters={saved_characters}, Skillpoints={saved_skillpoints}, "
        f"FieldSize={saved_field_size}, Terrain={saved_terrain}"
    )
unreal.log("Verified DT_FormatNative: Format5x16, 5 characters, 16 skillpoints, FieldSize=15, Terrain=150")
