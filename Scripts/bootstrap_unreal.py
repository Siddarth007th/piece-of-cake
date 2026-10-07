"""Run in UE Editor Python after the native module has compiled.

Creates actual .uasset materials/audio/Blueprints and an actual .umap. Idempotent:
existing hand-edited assets and maps are preserved. No binary placeholders.
"""
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
assets = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary

for folder in ("Characters", "Environments", "Materials", "VFX", "Audio", "UI",
               "Gameplay", "Levels", "Pickups", "Enemies", "Cinematics"):
    library.make_directory("/Game/" + folder)

material_path = "/Game/Materials/M_World"
if not library.does_asset_exist(material_path):
    material = assets.create_asset("M_World", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    editing = unreal.MaterialEditingLibrary
    tint = editing.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -650, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(.2, .3, .4, 1))
    glow = editing.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -650, 240)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0)
    roughness = editing.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 400)
    roughness.set_editor_property("r", .82)
    multiply = editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 160)
    editing.connect_material_expressions(tint, "", multiply, "A")
    editing.connect_material_expressions(glow, "", multiply, "B")
    editing.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    editing.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    editing.recompile_material(material)
    library.save_loaded_asset(material)

tasks = []
for file in sorted((ROOT / "Content/Audio/Source").glob("*.wav")):
    if library.does_asset_exist("/Game/Audio/" + file.stem):
        continue
    task = unreal.AssetImportTask()
    task.filename = str(file)
    task.destination_path = "/Game/Audio"
    task.destination_name = file.stem
    task.automated = True
    task.replace_existing = False
    task.save = True
    tasks.append(task)
assets.import_asset_tasks(tasks)
for index in range(8):
    sound = library.load_asset(f"/Game/Audio/M_{index}")
    if sound:
        sound.set_editor_property("looping", True)
        library.save_loaded_asset(sound)

for name, folder, parent in (("BP_Nori", "Characters", "POCCharacter"),
                             ("BP_EchoObject", "Gameplay", "POCProp"),
                             ("BP_Guardian", "Enemies", "POCEnemy")):
    if library.does_asset_exist(f"/Game/{folder}/{name}"):
        continue
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/PieceOfCake." + parent))
    bp = assets.create_asset(name, "/Game/" + folder, unreal.Blueprint, factory)
    library.save_loaded_asset(bp)

level_path = "/Game/Levels/L_LongWayToCake"
if not library.does_asset_exist(level_path):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(level_path):
        raise RuntimeError("Could not create the journey map")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 52), unreal.Rotator(0, 90, 0))
    start.set_actor_label("Nori — the beginning")
    levels.save_current_level()

required = [material_path, level_path] + [f"/Game/Audio/M_{i}" for i in range(8)]
missing = [path for path in required if not library.does_asset_exist(path)]
if missing:
    raise RuntimeError("Bootstrap incomplete: " + ", ".join(missing))
unreal.log("PIECE OF CAKE: asset bootstrap complete. Open L_LongWayToCake and press Play.")
