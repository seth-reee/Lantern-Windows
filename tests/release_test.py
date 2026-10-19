"""Exercise release retry/publication behavior without touching GitHub."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'packaging/windows/publish-release.sh'
MOCK = '''
import os, pathlib, shutil, sys
args = sys.argv[1:]
with open(os.environ['GH_CALLS'], 'a') as log:
    log.write(' '.join(args) + '\\n')
if args[:2] == ['release', 'view']:
    state = os.environ['GH_STATE']
    if state == 'absent': sys.exit(1)
    print('false' if state == 'published' else 'true')
if args[:2] == ['release', 'download']:
    target = pathlib.Path(args[args.index('--dir') + 1])
    for source in pathlib.Path(os.environ['GH_ASSETS']).iterdir():
        shutil.copy2(source, target / source.name)
'''


class ReleaseTest(unittest.TestCase):
    def exercise(self, state, mismatch=False, tag='v0.1.0'):
        with tempfile.TemporaryDirectory() as root:
            root = Path(root)
            assets = root / 'assets'
            assets.mkdir()
            name = 'Lantern-Windows-x64.zip'
            (assets / name).write_bytes(b'tested zip')
            checksum = hashlib.sha256(b'tested zip').hexdigest()
            (assets / 'SHA256SUMS.txt').write_text(f'{checksum}  {name}\n')
            remote = root / 'remote'
            remote.mkdir()
            for file in assets.iterdir():
                (remote / file.name).write_bytes(file.read_bytes())
            if mismatch:
                (remote / name).write_bytes(b'different published bytes')
            binary = root / 'gh'
            binary.write_text(f'#!{sys.executable}\n' + MOCK)
            binary.chmod(0o755)
            calls = root / 'calls'
            env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ['PATH'],
                       GH_STATE=state, GH_ASSETS=str(remote), GH_CALLS=str(calls))
            result = subprocess.run(['bash', str(SCRIPT), tag, str(assets)],
                                    env=env, capture_output=True, text=True)
            return result, calls.read_text() if calls.exists() else ''

    def test_new_release_is_draft_until_verified(self):
        result, calls = self.exercise('absent')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--draft --title', calls)
        self.assertLess(calls.index('release download'), calls.index('release edit'))
        self.assertIn('--draft=false', calls)

    def test_existing_published_release_is_verified_without_upload(self):
        result, calls = self.exercise('published')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn('release upload', calls)
        self.assertNotIn('release edit', calls)

    def test_mismatched_published_release_is_never_overwritten(self):
        result, calls = self.exercise('published', mismatch=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('release upload', calls)

    def test_draft_can_resume(self):
        result, calls = self.exercise('draft')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn('release create', calls)
        self.assertIn('release upload', calls)

    def test_invalid_tag_cannot_publish(self):
        result, calls = self.exercise('absent', tag='main')
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(calls, '')


if __name__ == '__main__':
    unittest.main()
