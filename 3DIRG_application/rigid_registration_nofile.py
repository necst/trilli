#!/usr/bin/env python3

import os
import sys
import argparse
import numpy as np

# Add build directory to sys.path to import the pybind11 wrapper
sys.path.append("./build")
import trilli_wrapper

# Import dataloader from same folder
import dataloader
from dataloader import load_nii_gz
from dataloader import read_volume_with_datalayout, write_volume_with_datalayout

def estimate_background_mode(data):
    data = data.astype(np.uint8)
    values, counts = np.unique(data, return_counts=True)
    return values[np.argmax(counts)]

def replace_background(data, old_bg_value, new_bg_value):
    data = data.astype(np.uint8)
    data[data == old_bg_value] = new_bg_value
    return data

# Fixed parameters
RANGE_TX = 50
RANGE_TY = 50
RANGE_ANG = 1.0
TARGET_SIZE = 512
BORDER_PADDING = 0

# Argument parser
parser = argparse.ArgumentParser(description="Run rigid registration using trilli_wrapper.")
parser.add_argument("--ref", required=True, help="Path to the reference volume (.nii.gz)")
parser.add_argument("--flt", required=True, help="Path to the floating volume (.nii.gz)")
parser.add_argument("--output_folder", default="./output_folder/", help="Output folder path")
parser.add_argument("--save_nifti", action="store_true", help="Save registered volume as NIfTI")
parser.add_argument("--output_folder_nifti", default="./output_folder/", help="Output folder path for NIfTI files")

args = parser.parse_args()

# Load volumes
ref_volume = load_nii_gz(args.ref)
flt_volume = load_nii_gz(args.flt)

# Estimate and normalize background
bg_ref = estimate_background_mode(ref_volume)
bg_flt = estimate_background_mode(flt_volume)
print(f"Estimated background - ref: {bg_ref}, flt: {bg_flt}")
if bg_ref != bg_flt:
    print("ALERT - different backgrounds - making them homogeneous by replacing REF values.")
    ref_volume = replace_background(ref_volume, old_bg_value=bg_ref, new_bg_value=bg_flt)

if ref_volume.shape != flt_volume.shape:
    raise ValueError(f"Input volumes must have same shape, got {ref_volume.shape} vs {flt_volume.shape}")

original_shape = ref_volume.shape
orig_width, orig_height, orig_depth = original_shape
# Pad to target resolution (xy)
ref_volume = dataloader.to_resolution(ref_volume, TARGET_SIZE, TARGET_SIZE)
flt_volume = dataloader.to_resolution(flt_volume, TARGET_SIZE, TARGET_SIZE)

depth = ref_volume.shape[2]
padding = (32 - (depth % 32)) % 32
total_depth = depth + padding
width = ref_volume.shape[0]
height = ref_volume.shape[1]

print(f"Volume shape after resolution adjustment: {ref_volume.shape}, dtype: {ref_volume.dtype}")

# Convert to uint8
if ref_volume.dtype != np.uint8:
    ref_volume = ref_volume.astype(np.uint8)
    flt_volume = flt_volume.astype(np.uint8)

# Create interlaced flat buffers
ref_flat, _, _ = dataloader.read_volume_with_datalayout_from_array(ref_volume, border_padding=BORDER_PADDING, depth_padding=padding)
flt_flat, _, _ = dataloader.read_volume_with_datalayout_from_array(flt_volume, border_padding=BORDER_PADDING, depth_padding=padding)

# Save input volumes (before registration)
write_volume_with_datalayout(ref_flat, width, height, total_depth, os.path.join(args.output_folder, "input_ref"))
write_volume_with_datalayout(flt_flat, width, height, total_depth, os.path.join(args.output_folder, "input_flt"))

# Run registration
print("Running rigid registration...")
registered_flat = trilli_wrapper.run_rigid_registration_trilli_from_data(
    ref_flat,
    flt_flat,
    args.output_folder,
    total_depth,
    RANGE_TX,
    RANGE_TY,
    RANGE_ANG
)

# Save registered buffer as PNG slices
write_volume_with_datalayout(registered_flat, width, height, total_depth, os.path.join(args.output_folder, "output", "registered"))

if args.save_nifti:
    # Rebuild volume from interleaved flat buffer
    registered_array = np.zeros((width, height, total_depth), dtype=np.uint8)
    slice_area = width * height
    for z in range(total_depth):
        slice_flat = [registered_flat[i * total_depth + z] for i in range(slice_area)]
        registered_array[:, :, z] = np.array(slice_flat, dtype=np.uint8).reshape((width, height), order='C')

    # Remove padding in depth
    registered_array = registered_array[:, :, :depth]

    # Remove padding in width and height (center crop)
    pad_x = width - orig_width
    pad_y = height - orig_height
    crop_x_start = pad_x // 2
    crop_y_start = pad_y // 2
    crop_x_end = crop_x_start + orig_width
    crop_y_end = crop_y_start + orig_height

    registered_array_cropped = registered_array[crop_x_start:crop_x_end, crop_y_start:crop_y_end, :]

    # Extract code from filename
    code = os.path.basename(args.flt).split('_')[0]

    # Save NIfTI
    filename = f"registered_{code}.nii.gz"
    out_path = os.path.join(args.output_folder_nifti, filename)
    dataloader.save_volume_as_nifti(registered_array_cropped, out_path, affine=np.eye(4))
else:
    print("Registration completed (no NIfTI file saved).")
