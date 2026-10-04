import importlib.util
import json
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location('data', Path(__file__).resolve().parents[1] / 'scripts/data.py')
data = importlib.util.module_from_spec(spec)
spec.loader.exec_module(data)

class DataTests(unittest.TestCase):
    def test_raw_regeneration_matches_committed(self):
        self.assertEqual(data.generate(), json.loads((data.ROOT / 'data/benchmark.json').read_text()))
    def test_missing_or_invalid_time_rejected(self):
        for value in ['', 'Execution Time: -1 seconds\n', 'Execution Time: 0 seconds\n', 'Execution Time: 1e999 seconds\n']:
            with self.assertRaises(ValueError):
                data.parse_output(value)
    def test_wrong_graph_rejected(self):
        with self.assertRaises(ValueError):
            data.parse_output('Execution Time: 0.1 seconds\nTotal edges: 9\n')
    def test_scientific_notation(self):
        self.assertEqual(data.parse_output('Execution Time: 1e-3 seconds\nTotal unique nodes: 10000\nTotal edges: 1000000\n'), .001)
    def test_concatenated_and_duplicate_records_rejected(self):
        valid = 'Execution Time: 0.1 seconds\nTotal unique nodes: 10000\nTotal edges: 1000000\n'
        for text in [valid + valid, valid + 'Execution Time: 0.2 seconds\n', valid + 'Total edges: 9\n', valid + 'Total unique nodes: 1\n']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                data.parse_output(text)
    def test_totals_must_be_whole_lines(self):
        for text in ['Execution Time: 0.1 seconds\nNot Total unique nodes: 10000\nTotal edges: 1000000\n', 'Execution Time: 0.1 seconds\nTotal unique nodes: 10000\nNot Total edges: 1000000\n']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                data.parse_output(text)
