#!/bin/bash

if [ ! -d "build" ]; then
    echo "Error: build/ directory does not exist."
    exit 1
fi

cp -r paper_fig build/
cp scripts/* build/
rm build/paper_fig/figure6/csv/*.csv
