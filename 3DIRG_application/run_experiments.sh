#!/bin/bash

NUM_REGS=50
OUTPUT_FILE=3DIRG_times.csv

echo "execTime" > $OUTPUT_FILE

for i in $(seq 1 $NUM_REGS)
do
    echo "Run $i/$NUM_REGS"
    ./exec.sh
    tail -n +2 TRILLI_pow.csv | cut -d',' -f1 >> $OUTPUT_FILE
done

echo "All runs completed. Execution times saved in $(realpath $OUTPUT_FILE)"
