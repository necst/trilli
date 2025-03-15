#!/bin/bash

if [ ! -d "build" ]; then
    echo "Error: build/ directory does not exist."
    exit 1
fi

cp -r paper_fig build/

# --- figure 6
cp scripts/gather_results_fig6.sh build/
rm build/paper_fig/figure6/csv/*.csv

# --- figure 7
cp scripts/gather_results_fig7.sh build/
rm build/paper_fig/figure7/csv/TRILLI_*.csv

# --- figure 8
cp scripts/gather_results_fig8.sh build/
cp scripts/generate_distortion.sh build/
rm build/paper_fig/figure8/data/result_IM0.png
rm build/paper_fig/figure8/data/float_IM0.png

echo "Packing complete"
