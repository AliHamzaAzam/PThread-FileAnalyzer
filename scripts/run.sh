#!/bin/bash

THREADS=(1 2 4 8 10 12 16 20 24 28 32)
AFFINITIES=("true" "false")

# Input files
TASK1FILE="Task1.txt"
TASK23FILE="Task2-3.txt"
TASK4FILE="Task4.log"
TASK5FILE="Task5.npy"

# Create output directories
mkdir -p ../bin ../results/{logs,debug}

time_wrapper() {
    local exe=$1
    local args=("${@:2}")

    # Capture time output and preserve exit code
    { time -p "$exe" "${args[@]}" >/dev/null 2> >(tee -a "$DEBUG_LOG"); } 2>&1 |
    awk '/^real/ {print $2}'
}

for exe in ../bin/*; do
    EXE_NAME=$(basename "$exe")
    LOG_FILE="../results/logs/${EXE_NAME}_results.csv"
    DEBUG_LOG="../results/debug/${EXE_NAME}_errors.log"

    # Initialize files
    echo "threads,affinity,time" > "$LOG_FILE"
    : > "$DEBUG_LOG"

    case $EXE_NAME in
        "Task1") INPUT_FILE=$TASK1FILE ;;
        "Task2-3") INPUT_FILE=$TASK23FILE ;;
        "Task4") INPUT_FILE=$TASK4FILE ;;
        "Task5") INPUT_FILE=$TASK5FILE ;;
        *) continue ;;
    esac

    for threads in "${THREADS[@]}"; do
        for affinity in "${AFFINITIES[@]}"; do
            TOTAL=0 COUNT=0

            for _ in {1..5}; do
                # Clear caches between runs
                sync && sudo purge  # macOS specific cache clear

                # Time execution with error capture
                RUNTIME=$(time_wrapper "$exe" "$INPUT_FILE" "$threads" "$affinity")

                if [[ $RUNTIME =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    TOTAL=$(awk "BEGIN {print $TOTAL + $RUNTIME}")
                    COUNT=$((COUNT + 1))
                else
                    echo "FAILED RUN: $EXE_NAME $threads $affinity" >> "$DEBUG_LOG"
                fi
            done

            # Record results
            if [ $COUNT -gt 0 ]; then
                AVG=$(awk "BEGIN {printf \"%.3f\", $TOTAL/$COUNT}")
                echo "$threads,$affinity,$AVG" >> "$LOG_FILE"
                echo "[SUCCESS] $EXE_NAME - ${threads}t ${affinity}: ${AVG}s"
            else
                echo "$threads,$affinity,ERROR" >> "$LOG_FILE"
                echo "[FAILURE] $EXE_NAME - ${threads}t ${affinity}: All runs failed"
            fi
        done
    done
done