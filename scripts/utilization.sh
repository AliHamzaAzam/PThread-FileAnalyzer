#!/bin/bash

THREADS=(10)
AFFINITIES=("true" "false")

# Input files
TASK1FILE="Task1.txt"
TASK23FILE="Task2-3.txt"
TASK4FILE="Task4.log"
TASK5FILE="Task5.npy"

# Create output directories
mkdir -p ../bin ../results/traces ../results/cpu_data

generate_trace() {
    local exe=$1
    local config_name=$2
    shift 2
    local args=("$@")

    local trace_file="../results/traces/${config_name}.trace"

    xctrace record --template "CPU Profiler" \
        --output "$trace_file" \
        --launch -- "$exe" "${args[@]}" >/dev/null 2>&1
}

extract_cpu_data() {
    local trace_file=$1
    local output_file=$2

    # Extract Table of Contents (TOC)
    xcrun xctrace export --input "$trace_file" --toc --output "${output_file}_toc.xml"

    # Extract CPU profiling data (if available)
    xcrun xctrace export --input "$trace_file" --xpath '/trace-toc/run/data/table[@schema="cpu-profile"]' --output "${output_file}_cpu.xml"
}

for exe in ../bin/*; do
    EXE_NAME=$(basename "$exe")

    case $EXE_NAME in
        "Task1") INPUT_FILE=$TASK1FILE ;;
        "Task2-3") INPUT_FILE=$TASK23FILE ;;
        "Task4") INPUT_FILE=$TASK4FILE ;;
        "Task5") INPUT_FILE=$TASK5FILE ;;
        *) continue ;;
    esac

    for threads in "${THREADS[@]}"; do
        for affinity in "${AFFINITIES[@]}"; do
            sync && sudo purge  # Clear caches
            CONFIG_NAME="${EXE_NAME}_threads${threads}_affinity${affinity}"
            TRACE_FILE="../results/traces/${CONFIG_NAME}.trace"
            CPU_DATA_FILE="../results/cpu_data/${CONFIG_NAME}"

            generate_trace "$exe" "$CONFIG_NAME" "$INPUT_FILE" "$threads" "$affinity"
            extract_cpu_data "$TRACE_FILE" "$CPU_DATA_FILE"
        done
    done
done