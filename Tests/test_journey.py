"""Geometric feasibility and project wiring checks; not a substitute for playing in UE."""
import importlib.util
import json
import math
from pathlib import Path
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
DATA = json.loads((ROOT / "Content/Data/journey.json").read_text())
POINTS = DATA["points"]


class JourneyTests(unittest.TestCase):
    def test_regeneration_is_deterministic(self):
        spec = importlib.util.spec_from_file_location("generate_journey", ROOT / "Scripts/generate_journey.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertEqual(module.generate(), DATA)

    def test_every_required_gap_has_airtime_margin_at_run_speed(self):
        movement = DATA["movement"]
        for a, b in zip(POINTS, POINTS[1:]):
            dz = b["position"][2] - a["position"][2]
            # Reserve 45 cm on both platforms for capsule radius and imperfect timing.
            distance = math.dist(a["position"][:2], b["position"][:2])
            gap = distance - a["size"][0] / 2 - b["size"][0] / 2 + 90
            lateral = 300 if a["kind"] == "moving" or b["kind"] == "moving" else 0
            gap = math.hypot(gap, lateral)
            discriminant = movement["jump_speed"] ** 2 - 2 * movement["gravity"] * dz
            self.assertGreater(discriminant, 0, f"Unreachable height at platform {b['index']}")
            flight = (movement["jump_speed"] + math.sqrt(discriminant)) / movement["gravity"]
            self.assertLess(gap, movement["run_speed"] * flight, f"Gap {a['index']} → {b['index']} is too long")

    def test_checkpoints_are_safe_and_never_more_than_eight_platforms_apart(self):
        checkpoints = [p for p in POINTS if p["checkpoint"]]
        self.assertEqual(len(checkpoints), 24)
        self.assertTrue(all(p["kind"] == "ground" for p in checkpoints))
        self.assertEqual(checkpoints[0]["index"], 0)
        self.assertTrue(all(b["index"] - a["index"] <= 8 for a, b in zip(checkpoints, checkpoints[1:])))
        self.assertLessEqual(POINTS[-1]["index"] - checkpoints[-1]["index"], 8)

    def test_echo_bridges_have_accessible_nodes_and_enough_time(self):
        nodes = {p["echo_node"]: p for p in POINTS if p["echo_node"] >= 0}
        groups = {p["group"] for p in POINTS if p["kind"] == "echo"}
        self.assertEqual(set(nodes), groups)
        for group in groups:
            bridges = [p for p in POINTS if p["group"] == group]
            self.assertEqual(nodes[group]["kind"], "ground")
            self.assertEqual(nodes[group]["index"] + 1, bridges[0]["index"])
            self.assertEqual(len(bridges), 2)
            # Generous allowance for interaction, lining up and both jumps.
            length = sum(math.dist(a["position"], b["position"]) for a,b in zip([nodes[group]] + bridges, bridges + [POINTS[bridges[-1]["index"]+1]]))
            self.assertLess(length / DATA["movement"]["run_speed"] + 4, 18)

    def test_secrets_are_optional_and_reachable(self):
        secrets = [p for p in POINTS if p["secret"]]
        self.assertEqual(len(secrets), 8)
        for point in secrets:
            self.assertEqual(point["kind"], "ground")
            gap = 900 - point["size"][1] / 2 - 250 + 90
            flight = (650 + math.sqrt(650**2 - 2*1568*60))/1568
            self.assertLess(gap, 620*flight)

    def test_route_has_all_eight_sections_and_an_unobstructed_ending(self):
        self.assertEqual(sorted(set(p["section"] for p in POINTS)), list(range(8)))
        self.assertEqual(len(POINTS), 192)
        self.assertTrue(all(p["enemy"] == -1 for p in POINTS[-24:]))
        self.assertEqual(POINTS[-1]["kind"], "ground")
        self.assertGreater(sum(math.dist(a["position"], b["position"]) for a,b in zip(POINTS,POINTS[1:])), 280000)
        self.assertFalse(DATA["timing_verified"], "Set this only after a timed playthrough")

    def test_original_audio_has_valid_nonempty_pcm(self):
        files = list((ROOT/"Content/Audio/Source").glob("*.wav"))
        self.assertEqual(len(files), 23)
        for file in files:
            with wave.open(str(file)) as audio:
                self.assertEqual(audio.getsampwidth(), 2)
                self.assertEqual(audio.getframerate(), 24000)
                self.assertGreater(audio.getnframes(), 2000)
                self.assertTrue(any(audio.readframes(5000)))

    def test_engine_and_frontend_versions_match(self):
        project = json.loads((ROOT/"PieceOfCake.uproject").read_text())
        package = json.loads((ROOT/"Web/package.json").read_text())
        self.assertEqual(project["EngineAssociation"], "5.8")
        self.assertTrue(next(p for p in project["Plugins"] if p["Name"] == "PixelStreaming2")["Enabled"])
        self.assertTrue(all(name.endswith("ue5.8") for name in package["dependencies"]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
