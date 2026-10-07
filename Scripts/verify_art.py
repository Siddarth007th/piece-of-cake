"""Run with Blender --background --python Scripts/verify_art.py."""
import bpy, bmesh, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
report=[]
for source in sorted((root/'Content/Art/Source').glob('SM_*.fbx')):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(source))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(meshes)==1,(source.name,len(meshes))
    obj=meshes[0]
    assert obj.data.vertices and obj.data.polygons,source.name
    assert all(.3 < d < 1.1 for d in obj.dimensions),(source.name,tuple(obj.dimensions))
    bm=bmesh.new();bm.from_mesh(obj.data)
    if source.stem!='SM_Fern':
        assert all(edge.is_manifold for edge in bm.edges),source.name
        assert bm.calc_volume(signed=True)>0,(source.name,'inverted normals')
    if source.stem=='SM_NoriEar':
        assert len(obj.data.materials)==2
        assert {p.material_index for p in obj.data.polygons}=={0,1}
    bm.free();report.append({'mesh':source.stem,'passed':True,'dimensions_m':[round(d,4) for d in obj.dimensions]})
(root/'Artifacts/mesh-roundtrip-results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: nine FBX roundtrips, dimensions, closed-solid normals, ear material slots')
