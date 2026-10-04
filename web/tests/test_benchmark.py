import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

WEB = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(WEB / 'scripts'))
spec = importlib.util.spec_from_file_location('benchmark', WEB / 'scripts/benchmark.py')
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)
VALID = 'Execution Time: 0.123 seconds\nTotal unique nodes: 10000\nTotal edges: 1000000\n'

class BenchmarkTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        shutil.copytree(WEB / 'data', self.root / 'web/data')
        shutil.copytree(WEB.parent / 'src', self.root / 'src')
        self.raw = self.root / 'web/data/raw'
        self.original = self.snapshot()
        self.calls = 0

    def snapshot(self):
        folder = self.root / 'web/data'
        return {str(p.relative_to(folder)): p.read_bytes() for p in folder.rglob('*') if p.is_file()}

    def fake_run(self, args, **kwargs):
        self.calls += 1
        if self.calls == self.fail_at:
            raise subprocess.CalledProcessError(1, args)
        return subprocess.CompletedProcess(args, 0, stdout=VALID, stderr='')

    def run_capture(self):
        with patch.object(benchmark, 'ROOT', self.root), patch.object(benchmark, 'RAW', self.raw), patch.object(benchmark.subprocess, 'run', side_effect=self.fake_run), patch.object(benchmark.subprocess, 'check_output', return_value='fake compiler\n'):
            benchmark.main()

    def test_late_subprocess_failure_preserves_all_evidence(self):
        self.fail_at = 23
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_capture()
        self.assertEqual(self.snapshot(), self.original)

    def test_publish_failure_rolls_back_all_evidence(self):
        self.fail_at = -1
        original_replace = os.replace
        failed = False
        def replace(source, target):
            nonlocal failed
            if Path(target) == self.root / 'web/data/benchmark.json' and not failed:
                failed = True
                raise OSError('injected JSON publication failure')
            return original_replace(source, target)
        with patch.object(os, 'replace', side_effect=replace):
            with self.assertRaises(OSError):
                self.run_capture()
        self.assertTrue(failed)
        self.assertEqual(self.snapshot(), self.original)

    def test_success_records_both_source_hashes(self):
        self.fail_at = -1
        self.run_capture()
        import hashlib
        import json
        metadata = json.loads((self.raw / 'machine.json').read_text())
        self.assertEqual(metadata['sourceSha256'], hashlib.sha256((self.root / 'src/Task1.cpp').read_bytes()).hexdigest())
        self.assertEqual(metadata['utilitySha256'], hashlib.sha256((self.root / 'src/utility.h').read_bytes()).hexdigest())
        generated = json.loads((self.root / 'web/data/benchmark.json').read_text())
        self.assertEqual(generated['machine'], metadata)
        self.assertEqual(generated['rows'][0]['samples'], [.123] * 5)

    def test_backup_retained_if_rollback_itself_fails(self):
        self.fail_at = -1
        original_replace = os.replace
        def replace(source, target):
            if Path(target) == self.root / 'web/data/benchmark.json' or Path(source).name == 'old-raw':
                raise OSError('injected publication and rollback failure')
            return original_replace(source, target)
        with patch.object(os, 'replace', side_effect=replace), self.assertRaises(OSError):
            self.run_capture()
        backups = list((self.root / 'web/data').glob('.capture-*/old-raw/machine.json'))
        self.assertEqual(len(backups), 1)
        self.assertEqual(backups[0].read_bytes(), self.original['raw/machine.json'])
