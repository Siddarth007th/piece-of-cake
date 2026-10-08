"""Render Nori's original game meshes as a reproducible native application icon."""
import bpy, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Content/Art/Source/Nori-source.blend'))
library={o.name[3:]:o.data.copy() for o in bpy.data.objects if o.name.startswith('SM_') and o.type=='MESH'}
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def mat(name,color,rough=.5):
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes['Principled BSDF'];n.inputs['Base Color'].default_value=(*color,1);n.inputs['Roughness'].default_value=rough;return m
cream=mat('Nori cream',(.87,.82,.66));teal=mat('Nori teal',(.11,.36,.36));ink=mat('Nori eyes',(.013,.024,.035),.23);red=mat('Ninja red',(.65,.095,.065));suit=mat('Ninja indigo',(.055,.095,.17));pale=mat('Muzzle',(.99,.89,.66));gold=mat('Gold',(.92,.47,.055),.26);white=mat('Glint',(1,1,1),.2)
def part(name,shape,pos,scale,material,rot=(0,0,0)):
 if shape in library:
  o=bpy.data.objects.new(name,library[shape].copy());bpy.context.collection.objects.link(o)
 else:
  bpy.ops.mesh.primitive_uv_sphere_add(segments=48,ring_count=32,radius=.5);o=bpy.context.object;o.name=name
  for p in o.data.polygons:p.use_smooth=True
 o.location=Vector(pos)/100;o.scale=scale;o.rotation_euler=[math.radians(v) for v in rot];o.data.materials.clear();o.data.materials.append(material);return o
part('Suit','NoriBody',(0,0,-5),(.51,.44,.55),suit)
part('Belly','Sphere',(20,0,-3),(.11,.31,.29),teal)
part('Head','NoriHead',(7,0,25),(.55,.52,.46),cream)
part('Muzzle','Sphere',(30,0,21),(.21,.3,.19),pale)
part('Nose','Sphere',(40,0,24),(.07,.1,.065),ink)
part('Scarf','Sphere',(2,0,10),(.51,.48,.12),red)
part('Ninja headband','Sphere',(6,0,41),(.565,.535,.095),red)
part('Crest','NoriScarf',(33,0,41),(.025,.075,.075),gold,(45,0,0))
for i in range(2):part('Headband ribbon','NoriScarf',(-28-i*12,12+i*7,40-i*7),(.36,.075,.035),red,(0,-20,15))
for side in (-1,1):
 ear=part('Ear','NoriEar',(0,side*16,60),(.16,.24,.55),cream,(side*19,-12,0));ear.data.materials.append(teal)
 for p in ear.data.polygons:p.material_index=1 if p.center.z>.22 else 0
 part('Eye rim','Sphere',(29,side*14,30),(.10,.185,.23),pale)
 part('Eye','Sphere',(33,side*14,30),(.07,.135,.175),ink)
 part('Iris','Sphere',(36,side*14,30),(.035,.082,.115),gold)
 part('Pupil','Sphere',(37,side*14,30),(.02,.044,.085),ink)
 part('Glint','Sphere',(38,side*14-2,33),(.015,.027,.033),white)
 part('Paw','NoriPaw',(20,side*24,side*5),(.17,.14,.21),cream)
for i in range(6):part('Red scarf tail','NoriScarf',(-22-i*9,5+math.sin(i)*2,10-i*3),(.14,.14-i*.009,.025),red,(0,-24,0))
def light(name,pos,power,size,color):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color;o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);o.location=pos;o.rotation_euler=(Vector((0,0,.35))-o.location).to_track_quat('-Z','Y').to_euler()
light('Key',(3,-4,5),420,4,(1,.86,.7));light('Fill',(3,3,1),330,3,(.6,.85,1));light('Rim',(-2,1,2),520,2,(.5,1,.85))
d=bpy.data.cameras.new('Icon Camera');c=bpy.data.objects.new('Icon Camera',d);bpy.context.collection.objects.link(c);c.location=(3,-1.45,1.03);target=Vector((.04,0,.37));c.rotation_euler=(target-c.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=1.18
s=bpy.context.scene;s.camera=c;s.render.engine='CYCLES';s.cycles.samples=64;s.cycles.use_denoising=True;s.render.resolution_x=s.render.resolution_y=1024;s.render.resolution_percentage=100;s.render.film_transparent=True;s.view_settings.view_transform='AgX';s.world.color=(.15,.15,.15);s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA'
s.render.filepath=str(ROOT/'Build/Mac/Resources/NoriPortrait.png');bpy.ops.render.render(write_still=True)
