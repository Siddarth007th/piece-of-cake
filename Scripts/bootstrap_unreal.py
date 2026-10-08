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
material = library.load_asset(material_path) if library.does_asset_exist(material_path) else assets.create_asset("M_World", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
editing = unreal.MaterialEditingLibrary
# Versioned graph: centimetre-scaled surfaces with clean mortar and subtle grain.
# The character uses style zero, preserving its clean, readable silhouette.
if library.get_metadata_tag(material, "POCSurfaceVersion") != "2":
    editing.delete_all_material_expressions(material)
    def scalar(name, value, x, y):
        node=editing.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
        node.set_editor_property("parameter_name",name); node.set_editor_property("default_value",value)
        return node
    tint=editing.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-900,0)
    tint.set_editor_property("parameter_name","Tint");tint.set_editor_property("default_value",unreal.LinearColor(.3,.3,.3,1))
    glow=scalar("Glow",.08,-900,350)
    roughness=scalar("Roughness",.86,-300,450)
    style=scalar("SurfaceStyle",0,-900,180)
    position=editing.create_material_expression(material,unreal.MaterialExpressionWorldPosition,-900,-250)
    normal=editing.create_material_expression(material,unreal.MaterialExpressionVertexNormalWS,-900,-400)
    pattern=editing.create_material_expression(material,unreal.MaterialExpressionCustom,-400,0)
    pattern.set_editor_property("output_type",unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for name in ("P","N","TintColor","Style"):
        item=unreal.CustomInput();item.set_editor_property("input_name",name);inputs.append(item)
    pattern.set_editor_property("inputs",inputs)
    pattern.set_editor_property("code",r"""
        if (Style < 0.5) return TintColor;
        float3 an=abs(N);
        float2 uv=an.z > .6 ? P.xy : (an.x > an.y ? P.yz : P.xz);
        float2 scale=Style < 1.5 ? float2(140,65) : Style < 2.5 ? float2(240,38) : float2(105,105);
        float row=floor(uv.y/scale.y);
        uv.x += Style < 1.5 ? fmod(abs(row),2.0)*70 : 0;
        float2 cell=floor(uv/scale);
        float2 f=frac(uv/scale);
        float2 edge=min(f,1-f)*scale;
        float mortar=smoothstep(1.0,3.2,min(edge.x,edge.y));
        float tone=frac(sin(dot(cell,float2(127.1,311.7)))*43758.5453);
        float grain=sin(uv.x*.12+sin(uv.y*.09)*2)*sin(uv.y*.18)*.022;
        if(Style>3.5) return TintColor*(.88+.10*sin(P.z*.042+sin(P.x*.014)*2)+grain);
        float detail=Style>1.5 && Style<2.5 ? sin(uv.x*.08+sin(uv.y*.12)*4)*.045 : grain;
        return TintColor*lerp(.43,.88+tone*.18+detail,mortar);
    """)
    for node,name in ((position,"P"),(normal,"N"),(tint,"TintColor"),(style,"Style")):
        editing.connect_material_expressions(node,"",pattern,name)
    emission=editing.create_material_expression(material,unreal.MaterialExpressionMultiply,0,240)
    editing.connect_material_expressions(pattern,"",emission,"A")
    editing.connect_material_expressions(glow,"",emission,"B")
    editing.connect_material_property(pattern,"",unreal.MaterialProperty.MP_BASE_COLOR)
    editing.connect_material_property(emission,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.connect_material_property(roughness,"",unreal.MaterialProperty.MP_ROUGHNESS)
    library.set_metadata_tag(material,"POCSurfaceVersion","2")
material.set_editor_property("used_with_instanced_static_meshes", True)
editing.recompile_material(material)
library.save_loaded_asset(material)

# Import original, editable mesh assets. Collision uses the existing gameplay shells.
library.make_directory("/Game/Art/Meshes")
mesh_sources = sorted((ROOT / "Content/Art/Source").glob("SM_*.fbx"))
if len(mesh_sources) != 9:
    raise RuntimeError("Expected nine original meshes. Run Blender with Scripts/build_art.py first.")
mesh_tasks = []
for file in mesh_sources:
    if library.does_asset_exist("/Game/Art/Meshes/" + file.stem):
        continue
    task = unreal.AssetImportTask()
    task.filename = str(file)
    task.destination_path = "/Game/Art/Meshes"
    task.destination_name = file.stem
    task.automated = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_materials = False
    options.import_textures = False
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    task.options = options
    mesh_tasks.append(task)
assets.import_asset_tasks(mesh_tasks)
for file in mesh_sources:
    path = "/Game/Art/Meshes/" + file.stem
    if not library.does_asset_exist(path):
        raise RuntimeError("Original mesh import failed: " + path)

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
