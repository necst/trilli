#!/usr/bin/env python3

import os
import sys
sys.path.append("./build")

import trilli_wrapper

VOLUME_DEPTH = 2
OUTPUT_FOLDER = "./output_folder/"
REF_FOLDER = "/home/gsorrentino/CT/"
FLT_FOLDER = "/home/gsorrentino/PET/"
RANGE_TX = 50
RANGE_TY = 50
RANGE_ANG = 1.0

# Override folder di floating se passato come argomento
if len(sys.argv) > 1:
    FLT_FOLDER = sys.argv[1]

print(f"Reference folder: {REF_FOLDER}")
print(f"Floating folder: {FLT_FOLDER}")
print(f"Registered output folder: {OUTPUT_FOLDER}")

os.makedirs(OUTPUT_FOLDER, exist_ok=True)

trilli_wrapper.run_rigid_registration_trilli(
    REF_FOLDER, FLT_FOLDER, OUTPUT_FOLDER, VOLUME_DEPTH, RANGE_TX, RANGE_TY, RANGE_ANG
)
