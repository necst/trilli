#!/bin/bash

if [ ! -d "build" ]; then
    echo "Error: build/ directory does not exist."
    exit 1
fi

cp -r paper_fig build/
cp scripts/gather_results_fig6.sh build/
rm build/paper_fig/figure6/csv/*.csv
