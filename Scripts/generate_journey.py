#!/usr/bin/env python3
"""Authored encounter beats, in centimetres, for eight enclosed districts."""
import json
import math
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
NAMES = ["Sunroot Vault", "Bellows Foundry", "Fern Archives", "Echo Sanctum",
         "Prism Grotto", "Tidal Gallery", "Clockwork Descent", "The Cake Chamber"]
POINTS_PER_SECTION = 24
RADIUS = 48000
HEIGHTS = [0, 300, 600, 900, 1200, 900, 1200, 1600]

def generate():
    points = []
    angle = 0.0
    # Three eight-beat rooms per district: entry, approach, encounter, recovery,
    # timing, vertical jump, distance jump, exit. They do not all require jumping.
    lengths = [1500, 1100, 1100, 1600, 1000, 850, 850, 1200]
    z_steps = [0, 0, 0, 0, 0, 200, 200, 200,
               200, 200, 160, 160, 160, 360, 360, 200,
               200, 200, 200, 200, 200, 400, 400, 300]
    for index in range(192):
        section, local = divmod(index, 24)
        beat, room = local % 8, local // 8
        length = lengths[beat] + (150 if section == 5 and beat == 3 else 0)
        gap = [0, -60, 130, -60, -30, 180, 680, 150][beat]
        if section == 0 and local < 8:
            gap = [0, -60, -60, -60, 100, 100, 260, 100][beat]
        if section == 6 and beat in (2, 3, 4): gap = 160
        if section == 7 and local >= 17:
            gap = -40
            length = 900 if local < 23 else 2400
        if section == 4 and beat in (2,3,4,5,6): length = max(length,1100); gap = min(gap,340)
        if section == 2 and local in (3, 19): length = 2200
        if section == 6 and 17 <= local <= 22: length = 900; gap = 130
        if index:
            angle += ((points[-1]['size'][0] + length) / 2 + gap) / RADIUS
        checkpoint = beat == 0 or (section == 0 and local == 3) or (section == 4 and beat == 4)
        kind, group, node = "ground", -1, -1
        if section in (2, 4, 5) and beat == 2: kind = "moving"
        if section in (3, 5) and local in (6, 7, 14, 15):
            kind, group = "echo", section * 100 + room
        if section in (3, 5) and local in (5, 13): node = section * 100 + room
        if section == 5 and beat == 2: kind = "lift"
        if section == 6 and (beat in (2, 3, 4, 6) or 17 <= local <= 22): kind = "crumble"
        challenge = "walk" if gap <= 0 else "jump"
        if beat == 5: challenge = "double_jump"
        if gap >= 600: challenge = "dash_jump"
        width = 1400 if beat in (0, 1, 3, 7) else 1050 if beat == 4 else 900
        if section == 2 and local in (3,19): width = 1850
        if section == 4 and beat in (2,3,4,5,6): width = 1250
        enemy = (section + room) % 3 if beat == 2 and kind == 'ground' else -1
        if section >= 4 and beat == 1: enemy = 0
        hazard = beat == 4 and section in (0, 1, 3, 5, 7)
        sweeper = beat == 4 and section in (2, 4, 6)
        bomb = beat == 3 and not (section == 0 and room == 0)
        if section==7 and 17<=local<=22:
            enemy = (local+1)%3
            hazard = sweeper = False
            bomb = local in (18,21)
            checkpoint = local==20
        if section == 0 and local < 8: enemy = -1; hazard = False
        if section == 2 and local in (3,19): enemy = -1; bomb = False
        if section == 6 and 17 <= local <= 22: enemy = -1; bomb = hazard = sweeper = False
        if section == 4 and beat in (2,3,4,5,6): enemy = -1; bomb = sweeper = False
        # Each district has an introduction, a quieter middle approach, and a finale.
        # Avoid stacking a bomb and an enemy on the same end stair.
        if section in (1,3,5) and room==1 and beat==4: hazard=False
        if section in (0,1,3,5) and room==1 and beat==7: bomb=True
        if section==7 and 17<=local<=22:
            enemy = -1 if local in (18,20,21) else enemy
        if checkpoint: enemy=-1
        points.append(dict(index=index, section=section, name=NAMES[section],
            position=[round(RADIUS*(math.cos(angle)-1),3), round(RADIUS*math.sin(angle),3), HEIGHTS[section]+(80 if section==0 and 5<=local<=7 else z_steps[local]*.4 if section==4 else 200+(local-16)*200 if section==7 and local>=17 else z_steps[local])],
            yaw=round(math.degrees(angle)+90,3), size=[length,width,200],
            kind=kind, group=group, echo_node=node, checkpoint=checkpoint,
            enemy=enemy, hazard=hazard, sweeper=sweeper, bomb=bomb,
            slide_gate=section in (1,2,6,7) and beat==1 and room==1,
            bounce=section in (0,2,4) and local==17, secret=local==23,
            challenge=challenge, incoming_gap=gap))
    # Prism stepping stones weave across the room instead of repeating a straight lane.
    offsets = [0, 0, -180, 180, -180, 180, -180, 0]
    for point in points:
        if point['section'] == 4:
            lateral = offsets[point['index'] % 8]
            yaw = math.radians(point['yaw'])
            point['position'][0] = round(point['position'][0] - math.sin(yaw)*lateral,3)
            point['position'][1] = round(point['position'][1] + math.cos(yaw)*lateral,3)
    return dict(version=3, seed=2718, name="The Long Way to Cake", sections=NAMES,
        target_minutes=[10,15], timing_verified=False, enclosed=True, shadows=False,
        movement=dict(run_speed=820,sprint_speed=1100,jump_speed=650,gravity=1568,
                      double_jump_speed=650,dash_speed=1550,dash_seconds=.2,dash_cooldown=.9),points=points)
if __name__=='__main__':
    dest=ROOT/'Content/Data/journey.json';dest.parent.mkdir(parents=True,exist_ok=True)
    dest.write_text(json.dumps(generate(),indent=2)+'\n')
    print('Wrote eight enclosed districts, 24 encounter rooms, 192 varied route beats.')
