"""Regenerate chart data using checked-in raw stdout only."""
import json
from pathlib import Path
import re
import statistics

ROOT = Path(__file__).resolve().parents[1]

def parse_output(text):
    # Count every record with a recognized prefix, including malformed duplicates.
    def record(label, pattern):
        records = [line for line in text.splitlines() if line.startswith(label + ':')]
        if len(records) != 1:
            raise ValueError(f'Expected exactly one {label} record')
        match = re.fullmatch(pattern, records[0])
        if not match:
            raise ValueError(f'Invalid {label} record')
        return match.group(1)

    value = float(record('Execution Time', r'Execution Time: ([0-9.eE+-]+) seconds'))
    if not 0 < value < float('inf'):
        raise ValueError('Execution time must be finite and positive')
    nodes = int(record('Total unique nodes', r'Total unique nodes: ([0-9]+)'))
    edges = int(record('Total edges', r'Total edges: ([0-9]+)'))
    if nodes != 10000 or edges != 1000000:
        raise ValueError('Incorrect graph totals for the fixed ring-graph protocol')
    return value

def generate(raw=None):
    raw = Path(raw) if raw is not None else ROOT / 'data/raw'
    machine = json.loads((raw / 'machine.json').read_text())
    rows = []
    for threads in [1, 2, 4, 8]:
        samples = [parse_output((raw / f'graph-{threads}-{i}.txt').read_text()) for i in range(1, machine['repetitions'] + 1)]
        rows.append({'threads': threads, 'seconds': statistics.median(samples), 'min': min(samples), 'max': max(samples), 'samples': samples})
    return {'machine': machine, 'rows': rows}

if __name__ == '__main__':
    (ROOT / 'data/benchmark.json').write_text(json.dumps(generate(), indent=2) + '\n')
