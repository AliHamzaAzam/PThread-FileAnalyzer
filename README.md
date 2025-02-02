
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