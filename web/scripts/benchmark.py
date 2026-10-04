"""Small local graph benchmark. No downloads, root privileges, or source edits."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import shutil
from data import generate, parse_output
import tempfile

ROOT = Path(__file__).resolve().parents[2]
RAW = ROOT / 'web/data/raw'
FLAGS = ['-std=c++17', '-pthread', '-O3', '-march=native', '-include', 'cstring']


def publish(capture, generated):
    """Publish a validated capture; restore old evidence if any replacement fails."""
    destination = RAW.parent
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'benchmark.json'
    # Stage on the destination filesystem so renames never cross devices.
    stage = Path(tempfile.mkdtemp(prefix='.capture-', dir=destination))
    keep_backup = False
    try:
        shutil.copytree(capture, stage / 'new-raw')
        (stage / 'new.json').write_text(json.dumps(generated, indent=2) + '\n')
        had_raw = RAW.exists()
        had_json = output.exists()
        if had_json:
            shutil.copy2(output, stage / 'old.json')
        moved_old = installed_new = False
        try:
            if had_raw:
                os.replace(RAW, stage / 'old-raw')
                moved_old = True
            os.replace(stage / 'new-raw', RAW)
            installed_new = True
            os.replace(stage / 'new.json', output)
        except BaseException:
            try:
                if installed_new:
                    shutil.rmtree(RAW)
                if moved_old:
                    os.replace(stage / 'old-raw', RAW)
                if had_json:
                    os.replace(stage / 'old.json', output)
                elif output.exists():
                    output.unlink()
            except BaseException as error:
                keep_backup = True
                raise OSError(f'Rollback failed; original evidence retained in {stage}') from error
            raise
    finally:
        if not keep_backup:
            shutil.rmtree(stage)


def main():
    with tempfile.TemporaryDirectory(prefix='pthread-results-') as tmp:
        tmp = Path(tmp)
        capture = tmp / 'raw'
        capture.mkdir()
        graph = tmp / 'graph.tsv'
        graph.write_text(''.join(f'{i % 10000}\t{(i + 1) % 10000}\n' for i in range(1000000)))
        binary = tmp / 'Task1'
        subprocess.run(['g++', *FLAGS, str(ROOT / 'src/Task1.cpp'), '-o', str(binary)], check=True, capture_output=True)
        metadata = {'date': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'platform': platform.system(), 'architecture': platform.machine(), 'kernel': platform.release(), 'cpu': next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')), 'unknown'), 'availableCpus': len(os.sched_getaffinity(0)), 'cpuQuota': Path('/sys/fs/cgroup/cpu.max').read_text().strip() if Path('/sys/fs/cgroup/cpu.max').exists() else 'unknown', 'compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0], 'flags': FLAGS, 'sourceSha256': hashlib.sha256((ROOT / 'src/Task1.cpp').read_bytes()).hexdigest(), 'utilitySha256': hashlib.sha256((ROOT / 'src/utility.h').read_bytes()).hexdigest(), 'dataset': {'name': 'Synthetic ring graph', 'edges': 1000000, 'nodes': 10000, 'bytes': graph.stat().st_size, 'sha256': hashlib.sha256(graph.read_bytes()).hexdigest(), 'generator': 'For i in range(1000000): write i % 10000, tab, (i + 1) % 10000, newline'}, 'affinity': False, 'repetitions': 5, 'warmupsPerConfiguration': 1, 'cache': 'No cache clearing; warm filesystem cache', 'timing': 'Program Execution Time: thread creation through joins, mutex merges, and top-ten selection; excludes mapping, chunk calculation and output', 'command': './Task1 graph.tsv THREADS false false'}
        for threads in [1, 2, 4, 8]:
            warmup = subprocess.run([str(binary), str(graph), str(threads), 'false', 'false'], check=True, capture_output=True, text=True)
            parse_output(warmup.stdout)
            for repeat in range(1, 6):
                result = subprocess.run([str(binary), str(graph), str(threads), 'false', 'false'], check=True, capture_output=True, text=True)
                parse_output(result.stdout)
                (capture / f'graph-{threads}-{repeat}.txt').write_text(result.stdout)
        (capture / 'machine.json').write_text(json.dumps(metadata, indent=2) + '\n')
        generated = generate(capture)
        publish(capture, generated)

if __name__ == '__main__':
    main()
