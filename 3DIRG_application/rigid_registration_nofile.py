#!/usr/bin/env python3

import os
import sys
import argparse
import numpy as np

# Add build directory to sys.path to import the pybind11 wrapper
sys.path.append("./build")
import trilli_wrapper

# Import dataloader from same folder
from dataloader import load_nii_gz

# Fixed parameters
RANGE_TX = 50
RANGE_TY = 50
RANGE_ANG = 1.0

# Argument parser
parser = argparse.ArgumentParser(description="Run rigid registration using trilli_wrapper.")
parser.add_argument("--ref", required=True, help="Path to the reference volume (.nii.gz)")
parser.add_argument("--flt", required=True, help="Path to the floating volume (.nii.gz)")
parser.add_argument("--output_folder", default="./output_folder/", help="Output folder path")
parser.add_argument("--save_nifti", action="store_true", help="Save registered volume as NIfTI")

args = parser.parse_args()

# Load volumes
ref_volume = load_nii_gz(args.ref)
flt_volume = load_nii_gz(args.flt)

if ref_volume.shape != flt_volume.shape:
    raise ValueError(f"Input volumes must have same shape, got {ref_volume.shape} vs {flt_volume.shape}")

depth = ref_volume.shape[2]
width = ref_volume.shape[0]
height = ref_volume.shape[1]

print(f"Loaded volumes with shape: {ref_volume.shape}, dtype: {ref_volume.dtype}")

# Convert to uint8 if necessary (your C++ function expects uint8_t)
if ref_volume.dtype != np.uint8:
    ref_volume = ref_volume.astype(np.uint8)
    flt_volume = flt_volume.astype(np.uint8)

# Flatten in Fortran order (slice after slice)
ref_flat = ref_volume.flatten(order='F').tolist()
flt_flat = flt_volume.flatten(order='F').tolist()

# Call C++ registration function
print("Running rigid registration...")
registered_flat = trilli_wrapper.run_rigid_registration_trilli_from_data(
    ref_flat,
    flt_flat,
    args.output_folder,
    depth,
    RANGE_TX,
    RANGE_TY,
    RANGE_ANG
)

# Convert back to 3D volume
registered_volume = np.array(registered_flat, dtype=np.uint8).reshape((width, height, depth), order='F')

# Save output if requested
if args.save_nifti:
    import nibabel as nib
    os.makedirs(args.output_folder, exist_ok=True)
    out_path = os.path.join(args.output_folder, "registered.nii.gz")
    nib.save(nib.Nifti1Image(registered_volume, affine=np.eye(4)), out_path)
    print(f"Registered volume saved to: {out_path}")
else:
    print("Registration completed (no file saved).")
