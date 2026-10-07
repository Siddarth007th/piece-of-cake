"""Original game meshes; run Blender --background --python Scripts/build_art.py.

Shapes use metres with one-unit bounds, exported at UE's centimetre scale.
Collision stays on the existing gameplay shells; these meshes are visual only.
The preview is rendered from these exact meshes, not a generated illustration.
"""
import bpy
import bmesh
import math
import json
import random
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Content/Art/Source'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
library = {}

def finish(name, obj, smooth=True):
    obj.name = 'SM_' + name
    bm = bmesh.new(); bm.from_mesh(obj.data); bmesh.ops.recalc_face_normals(bm, faces=bm.faces); bm.to_mesh(obj.data); bm.free()
    for p in obj.data.polygons:
        p.use_smooth = smooth
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    library[name] = obj
    return obj

def surface(name, deform, rings=24, sides=32):
    verts, faces = [], []
    for i in range(rings+1):
        t = math.pi * (.0001 + (1-.0002)*i/rings)
        for j in range(sides):
            a = 2*math.pi*j/sides
            verts.append(deform(t, a))
    for i in range(rings):
        for j in range(sides):
            a=i*sides+j; b=i*sides+(j+1)%sides
            faces.append((a,b,b+sides,a+sides))
    faces += [tuple(reversed(range(sides))), tuple(rings*sides+j for j in range(sides))]
    mesh = bpy.data.meshes.new(name); mesh.from_pydata(verts, [], faces); mesh.update()
    obj = bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
    # All assets share a 100 cm bounding cube before per-part scale.
    mins=[min(v.co[k] for v in mesh.vertices) for k in range(3)]
    maxs=[max(v.co[k] for v in mesh.vertices) for k in range(3)]
    for v in mesh.vertices:
        for k in range(3): v.co[k]=(v.co[k]-mins[k])/(maxs[k]-mins[k])-.5
    mesh.update()
    return finish(name,obj)

surface('NoriBody',lambda t,a:(.5*math.sin(t)*math.cos(a)*(1-.17*math.cos(t)), .5*math.sin(t)*math.sin(a)*(1-.2*math.cos(t)),.5*math.cos(t)))
surface('NoriHead',lambda t,a:(.5*math.sin(t)*math.cos(a)+.065*(math.sin(t)**2), .5*math.sin(t)*math.sin(a)*(1-.08*math.cos(t)), .5*math.cos(t)))
surface('NoriEar',lambda t,a:(.18*math.sin(t)*math.cos(a)-.15*math.cos(t)**2, .5*(math.sin(t)**1.5)*math.sin(a), .5*math.cos(t)),32,24)
surface('NoriPaw',lambda t,a:(.5*math.sin(t)*math.cos(a)+.08*math.sin(t)**2,.5*math.sin(t)*math.sin(a),.5*math.cos(t)))

def bevel_box(name, amount=.1):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj=bpy.context.object
    modifier=obj.modifiers.new('Soft stitched edges','BEVEL'); modifier.width=amount; modifier.segments=4
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    return finish(name,obj)
ear=library['NoriEar']
for slot in ('EarCream', 'EarTeal'):
    mat=bpy.data.materials.new(slot);ear.data.materials.append(mat)
for polygon in ear.data.polygons:
    polygon.material_index=1 if polygon.center.z > .22 else 0
bevel_box('NoriPack',.14)
bevel_box('NoriScarf',.1)
bevel_box('DressedStone',.055)
# Layered, weathered silhouettes replace perfect cones in the landscape.
surface('WeatheredRock',lambda t,a:(math.sin(t)*math.cos(a)*(1+.1*math.sin(a*5+t*3)),math.sin(t)*math.sin(a)*(1+.08*math.cos(a*3+t*4)),math.cos(t)),10,12)
# Fern leaf fans, kept low-poly for instancing along the entire journey.
verts=[]; faces=[]
for stem in range(7):
    angle=stem*2*math.pi/7
    direction=Vector((math.cos(angle),math.sin(angle),0)); side=Vector((-math.sin(angle),math.cos(angle),0))
    for leaf in range(1,7):
        t=leaf/7; center=direction*(t*.43)+Vector((0,0,.03+math.sin(t*2.2)*.46))
        for sign in (-1,1):
            width=(1-t)*.15; tip=center+side*sign*width+direction*.11+Vector((0,0,-.05))
            i=len(verts); verts += [tuple(center-direction*.045),tuple(tip),tuple(center+direction*.08),tuple(center+Vector((0,0,.022)))]
            faces += [(i,i+1,i+3),(i+1,i+2,i+3)]
mesh=bpy.data.meshes.new('Fern');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('Fern',mesh);bpy.context.collection.objects.link(obj);finish('Fern',obj,False)
# Closed-sided leaves remain visible from underneath without globally two-sided materials.
bm=bmesh.new();bm.from_mesh(obj.data)
faces=bmesh.ops.duplicate(bm,geom=list(bm.faces))['geom']
bmesh.ops.reverse_faces(bm,faces=[f for f in faces if isinstance(f,bmesh.types.BMFace)])
bm.to_mesh(obj.data);bm.free()

manifest=[]
for name,obj in library.items():
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_space_transform=False,mesh_smooth_type='FACE')
    manifest.append({'name':obj.name,'vertices':len(obj.data.vertices),'triangles':sum(len(p.vertices)-2 for p in obj.data.polygons),'source':'Scripts/build_art.py','bounds_m':[round(v,4) for v in obj.dimensions]})
(ROOT/'Content/Art/mesh_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
for obj in library.values(): obj.hide_render=True; obj.hide_viewport=True

def material(name,color,rough=.7):
    mat=bpy.data.materials.new(name);mat.diffuse_color=(*color,1);mat.use_nodes=True
    node=mat.node_tree.nodes.get('Principled BSDF');node.inputs['Base Color'].default_value=(*color,1);node.inputs['Roughness'].default_value=rough
    return mat
cream=material('Warm oat',(.87,.75,.52));pale=material('Muzzle',(.99,.89,.66));teal=material('Teal markings',(.035,.31,.29));ink=material('Ink',(.008,.018,.022),.22);gold=material('Amber iris',(.92,.47,.055),.28);white=material('Catchlight',(1,.96,.85),.15);red=material('Paprika scarf',(.68,.045,.022));leather=material('Leather',(.18,.095,.035));bronze=material('Brass',(.8,.42,.09),.3)

def part(name,shape,pos,scale,mat,rotation=(0,0,0)):
    if shape in library:
        obj=bpy.data.objects.new(name,library[shape].data.copy());bpy.context.collection.objects.link(obj)
    else:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=.5);obj=bpy.context.object;obj.name=name
        for p in obj.data.polygons:p.use_smooth=True
    obj.location=Vector(pos)/100;obj.scale=scale;obj.rotation_euler=[math.radians(x) for x in rotation];obj.data.materials.clear();obj.data.materials.append(mat)
    return obj
part('Body','NoriBody',(0,0,-5),(.51,.44,.55),cream)
part('Belly','Sphere',(20,0,-3),(.15,.32,.36),pale)
part('Head','NoriHead',(7,0,25),(.55,.52,.46),cream)
part('Muzzle','Sphere',(30,0,21),(.21,.3,.19),pale)
part('Nose','Sphere',(40,0,24),(.07,.10,.065),ink)
part('Pack','NoriPack',(-23,0,0),(.23,.33,.35),leather)
part('Pack flap','NoriPack',(-35,0,8),(.055,.34,.13),leather)
part('Pack clasp','NoriPack',(-39,0,3),(.025,.07,.10),bronze)
part('Scarf collar','Sphere',(2,0,10),(.51,.48,.12),red)
for side in (-1,1):
    ear=part('Ear','NoriEar',(0,side*16,60),(.16,.24,.55),cream,(side*-16,-10,0))
    ear.data.materials.append(teal)
    for polygon in ear.data.polygons:
        polygon.material_index = 1 if polygon.center.z > .22 else 0
    part('Eye socket','Sphere',(29,side*14,30),(.10,.185,.23),pale)
    part('Eye','Sphere',(33,side*14,30),(.07,.135,.175),ink)
    part('Iris','Sphere',(36,side*14,30),(.035,.082,.115),gold)
    part('Pupil','Sphere',(37,side*14,30),(.02,.044,.085),ink)
    part('Glint','Sphere',(38,side*14-2,33),(.015,.027,.033),white)
    part('Foot','NoriPaw',(6,side*15,-34),(.29,.20,.16),teal)
    part('Paw','NoriPaw',(13,side*24,-3),(.17,.14,.21),cream)
for index in range(6):
    part('Scarf tail','NoriScarf',(-22-index*9,5+math.sin(index)*2,10-index*4),(.14,.14-index*.009,.025),red,(0,-24,0))
# Reproducible studio preview of the mesh parts above. Not an Unreal screenshot.
ground=material('Studio floor',(.055,.095,.11))
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.425));bpy.context.object.data.materials.append(ground)
def area(name,pos,power,size,color):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj);obj.location=pos;obj.rotation_euler=(Vector((0,0,.2))-obj.location).to_track_quat('-Z','Y').to_euler()
area('Warm key',(3,-4,5),550,4,(1,.86,.66));area('Cool fill',(1,4,2),350,3,(.58,.83,1));area('Rim',(-3,1,3),600,3,(.58,1,.84))
data=bpy.data.cameras.new('Camera');camera=bpy.data.objects.new('Camera',data);bpy.context.collection.objects.link(camera);camera.location=(2.5,-3,1.35);target=Vector((0,0,.18));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();data.type='ORTHO';data.ortho_scale=1.8;scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1100;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.world.color=(.15,.15,.15);scene.view_settings.view_transform='AgX'
(ROOT/'Artifacts').mkdir(exist_ok=True);scene.render.filepath=str(ROOT/'Artifacts/Nori-mesh-preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Nori-source.blend'))
bpy.ops.render.render(write_still=True)
print('POC original art exported:',len(manifest),'meshes')
