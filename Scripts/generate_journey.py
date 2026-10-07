#!/usr/bin/env python3
"""Deterministic source data for the continuous route. Centimetres, Z-up."""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES = ["The Meadow", "Forgotten Ruins", "Ancient Forest", "The Listening Temple",
         "Underground Caverns", "The Sunken City", "The Long Way Down", "One Last Climb"]
HEIGHTS = [0, 80, 260, 1050, 550, 400, 850, 1400, 2350]
POINTS_PER_SECTION = 24
RADIUS = 48000


def generate():
    points = []
    for index in range(POINTS_PER_SECTION * 8):
        section, local = divmod(index, POINTS_PER_SECTION)
        angle = index / (POINTS_PER_SECTION * 8) * 2 * math.pi
        z = HEIGHTS[section] + (HEIGHTS[section + 1] - HEIGHTS[section]) * local / POINTS_PER_SECTION
        kind, group = "ground", -1
        checkpoint = local in (0, 8, 16)
        if section in (1, 2, 4, 5) and local in (5, 13, 21):
            kind = "moving"
        if section in (3, 5) and local in (5, 6, 13, 14):
            kind, group = "echo", section * 100 + (0 if local < 10 else 1)
        if section == 6 and not checkpoint and local not in (1, 20, 23):
            kind = "crumble"
        node = section * 100 + (0 if local < 10 else 1) if section in (3, 5) and local in (4, 12) else -1
        points.append({
            "index": index, "section": section, "name": NAMES[section],
            "position": [round(RADIUS * (math.cos(angle) - 1), 3), round(RADIUS * math.sin(angle), 3), round(z, 3)],
            "yaw": round(math.degrees(angle) + 90, 3),
            "size": [1340, 900 if section not in (0, 7) else 1200, 200],
            "kind": kind, "group": group, "echo_node": node,
            "checkpoint": checkpoint,
            "enemy": (section + local) % 3 if section in (1, 2, 3, 4, 5, 6) and local in (3, 11, 19) else -1,
            "hazard": section in (1, 3, 4, 6) and local in (7, 15, 22),
            "slide_gate": section in (3, 6) and local in (10, 18),
            "bounce": section in (0, 2, 6) and local in (6, 17),
            "secret": local == 20,
        })
    return {"version": 1, "seed": 2718, "name": "The Long Way to Cake", "sections": NAMES,
            "target_minutes": [10, 15], "timing_verified": False,
            "movement": {"run_speed": 620, "sprint_speed": 840, "jump_speed": 650, "gravity": 1568},
            "points": points}


if __name__ == "__main__":
    dest = ROOT / "Content/Data/journey.json"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(generate(), indent=2) + "\n")
    print(f"Wrote {dest}: 192 platforms, 8 sections, 24 checkpoints.")
