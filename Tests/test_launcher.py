"""Packaged game regression: launching an executable must not need the editor."""
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ue', ROOT / 'Scripts/ue.py')
ue = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ue)

class PackagedLauncherTests(unittest.TestCase):
    def test_packaged_stream_skips_engine_and_forwards_required_flags(self):
        with tempfile.TemporaryDirectory() as folder:
            executable = Path(folder) / 'Piece of Cake'; executable.touch()
            with patch.dict(os.environ, {'GAME_EXECUTABLE': str(executable)}), patch.object(sys, 'argv', ['ue.py','stream']), patch.object(ue,'engine_root',side_effect=AssertionError('Packaged games must be independent of UE')), patch.object(ue,'run') as launch:
                ue.main()
            command = launch.call_args.args[0]
            self.assertEqual(command[0], executable.resolve())
            self.assertIn('-PixelStreamingID=piece-of-cake', command)
            self.assertIn('-PixelStreamingEncoderCodec=H264', command)
            self.assertIn('-AudioMixer', command)
            self.assertNotIn(ue.PROJECT, command)

    def test_missing_packaged_game_fails_without_starting_editor(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.dict(os.environ, {'GAME_EXECUTABLE':str(Path(folder)/'missing')}), patch.object(sys,'argv',['ue.py','stream']), patch.object(ue,'engine_root',side_effect=AssertionError('Must fail on missing executable')), patch.object(ue,'run') as launch:
                with self.assertRaisesRegex(SystemExit,'GAME_EXECUTABLE does not exist'): ue.main()
                launch.assert_not_called()

    def test_editor_workflow_still_requires_engine(self):
        with patch.dict(os.environ, {'GAME_EXECUTABLE':''}), patch.object(sys,'argv',['ue.py','stream']), patch.object(ue,'engine_root',side_effect=SystemExit('Engine missing')), patch.object(ue,'run') as launch:
            with self.assertRaisesRegex(SystemExit,'Engine missing'): ue.main()
            launch.assert_not_called()

if __name__ == '__main__': unittest.main()
