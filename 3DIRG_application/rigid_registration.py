#!/usr/bin/env python3

import os
import sys
import argparse

# Add build directory to sys.path to import the pybind11 wrapper
sys.path.append("./build")
import trilli_wrapper

# Fixed parameters (can be exposed as CLI args if needed)
VOLUME_DEPTH = 246
RANGE_TX = 50
RANGE_TY = 50
RANGE_ANG = 1.0

# Argument parser setup
parser = argparse.ArgumentParser(description="Run rigid registration using trilli_wrapper.")
parser.add_argument("--ref_folder", "-rf" ,type=str, help="Path to the reference volume folder")
parser.add_argument("--flt_folder", "-ff" ,type=str, help="Path to the floating volume folder")
parser.add_argument("--output_folder", type=str, default="./output_folder/",
                    help="Path to the output folder (default: ./output_folder/)")

args = parser.parse_args()

# Extract arguments into variables
ref_folder = args.ref_folder
flt_folder = args.flt_folder
output_folder = args.output_folder

# Print configuration summary
print(f"Reference folder: {ref_folder}")
print(f"Floating folder: {flt_folder}")
print(f"Registered output folder: {output_folder}")

# Ensure output directory exists
os.makedirs(output_folder, exist_ok=True)

# Call the C++ registration function via pybind11 wrapper
trilli_wrapper.run_rigid_registration_trilli(
    ref_folder,
    flt_folder,
    output_folder,
    VOLUME_DEPTH,
    RANGE_TX,
    RANGE_TY,
    RANGE_ANG
)
