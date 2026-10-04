
---

# PThread File Analyzer

**PThread File Analyzer** is a multi-threaded file analyzer that efficiently processes large files by leveraging POSIX threads. It reads input files using memory-mapped I/O and computes a variety of statistics about the file's content, such as word count, character count, and line count. Additionally, it offers optional core affinity, enabling users to bind threads to specific CPU cores for improved performance. The analysis results can also be saved to a file for future reference.

---

## Features

- **Multi-threaded Processing:**  
  Utilizes POSIX threads to distribute the workload across multiple threads, enhancing performance on multi-core systems.

- **Memory Mapping:**  
  Efficiently reads input files using memory-mapped I/O for faster processing.

- **Statistics Computation:**  
  Calculates various statistics including:
    - Word count
    - Character count
    - Line count

- **Optional Core Affinity:**  
  Allows users to specify CPU cores to which threads should be pinned, optimizing CPU utilization.

- **Result Export:**  
  Provides an option to save the analysis results to a file for later review.

---

## Requirements

- **Compiler:**  
  A C++ compiler with C++17 support (e.g., **g++-14**).

- **Operating System:**  
  A POSIX-compliant operating system.  
  **Note:** This project has been developed and tested on **macOS**.

- **Libraries:**  
  The pthreads library (commonly included on POSIX systems).

---

## Compilation

Use the following sample command to compile the source files. Modify it as needed for your specific file names and paths.

```bash
g++-14 -std=c++17 -pthread -O3 -march=native src/[source_file].cpp -o [output_file]
```

Replace `[source_file]` with your actual source file name and `[output_file]` with your desired executable name.

---

## Usage

Run the program from the command line using the following syntax:

```bash
./[output_file] <file_path> <num_threads> [core_affinity] [save_to_file]
```

- **`[output_file]`**: The compiled executable.
- **`<file_path>`**: Path to the input file to be analyzed.
- **`<num_threads>`**: Number of threads to use.
- **`[core_affinity]`**: *(Optional)* Enable core affinity.
- **`[save_to_file]`**: *(Optional)* Save the results to a file.

---

## Scripts

The `scripts` directory contains several shell scripts to automate compilation and testing. To run a script, execute:

```bash
./scripts/[script_name].sh
```

Replace `[script_name]` with the name of the script you wish to run.

---

## Datasets

The following datasets can be used to test the program. **Before running the program, please rename the downloaded files as specified below.**

### Task 1

- **Dataset URL:**  
  [Orkut Community Data](https://snap.stanford.edu/data/bigdata/communities/com-orkut.ungraph.txt.gz)
- **Required File Name:**  
  `Task1.txt`

### Task 2-3

- **Dataset URL:**  
  [MultiUN Data](https://www.euromatrixplus.net/media/un-release/multiUN.en.tgz)  
  *(Note: Combine into a single file using the provided `extract.py` script)*
- **Required File Name:**  
  `Task2-3.txt`

### Task 4

- **Dataset URL:**  
  [HDFS Log Data](https://github.com/logpai/loghub/blob/master/HDFS#hdfs_v1)
- **Required File Name:**  
  `Task4.log`

### Task 5

- **Dataset URL:**  
  *(Generated using the `matrix.py` script)*
- **Required File Name:**  
  `Task5.npy`

---

This documentation should help you get started with the **PThread File Analyzer** project. Enjoy efficient file analysis with multi-threaded processing and optimized performance on macOS!
## Results site

The static results site in `web/` uses Vite and vanilla TypeScript. It explains the
actual implementation and presents a small, reproducible Linux graph benchmark.
The original macOS report observations appear separately; they are not mixed into
Linux scaling charts. No historical CSVs are present in this repository.

Requirements: Node.js 22.18+ (native TypeScript test support), npm, and Python 3.
To collect new measurements, also install a C++17 g++ compiler on Linux. The site
itself does not require a compiler or benchmark datasets.

```bash
cd web
npm ci --ignore-scripts
npm run dev
npm run typecheck
npm test
npm run build
npm run preview
```

Vite prints the local URL. `npm run build` writes `web/dist/`. The site uses no
external fonts, analytics, APIs, or runtime service.

Regenerate the checked-in `web/data/benchmark.json` from raw stdout and machine
metadata without rerunning measurements:

```bash
cd web
npm run data
npm test
```

Collect a fresh small synthetic graph benchmark and regenerate JSON:

```bash
cd web
npm run benchmark
npm test
npm run build
```

After all samples validate, this replaces `web/data/raw/graph-*.txt`,
`web/data/raw/machine.json`, and
`web/data/benchmark.json`. The script generates 1,000,000 ring edges over 10,000
nodes in a temporary directory, checks both output totals, and removes generated
input/binary files afterward. It compiles unchanged `src/Task1.cpp` with
`g++ -std=c++17 -pthread -O3 -march=native -include cstring`. The forced include
supplies the existing source's missing `memchr` declaration on Linux. It does not
modify C++ sources. Compiler version, flags, dataset/source/header hashes, allowed CPUs,
cgroup quota, date and command are saved with the raw output.

Each configuration (1, 2, 4, 8 threads, affinity off) runs once for warmup and five
times for measurement, in ascending order. Cache is not cleared. Charts use the
median program `Execution Time`, covering worker creation, joins, mutex merges and
top-ten selection, but excluding mapping, chunk setup and output. Runtime is shown
in milliseconds; speedup is the one-thread median divided by each median;
efficiency is speedup divided by thread count, as a percentage. The table includes
min/max values. These short, warm-cache cloud measurements are not representative
of the full Orkut dataset or the original Mac. The recorded run had a four-CPU container quota. A rerun records its own quota;
resource limits and other host activity can influence timings.

The report's profile durations are transcribed from PDF pages 9, 14 and 19; the
machine description is on page 2. `scripts/utilization.sh` specifies 10 threads.
Those observations have no one-thread baseline and are excluded from scaling
calculations. macOS affinity tags do not mean hard CPU pinning; no verified
cross-platform affinity comparison is claimed.

A served browser smoke check is available using an already installed Playwright
and Chromium (the script does not download or install either):

```bash
cd web
npm run preview -- --port 4175
# In another terminal, use your installed module and browser paths:
PLAYWRIGHT_MODULE=/opt/codex/cua_node/lib/node_modules/playwright \
CHROME_PATH=/usr/bin/chromium node tests/browser.mjs
```

This checks desktop and mobile overflow, chart switching, accessible SVG labels,
measurement-table expansion, minimum chart/diagram label size, metadata download, navigation, resize behavior,
and page/console errors against the production build at 1280, 768, 390 and 320 px.
The build uses relative asset URLs so it also works under a repository subpath.
