import argparse
import os
import numpy as np
import nibabel as nib
import imageio.v2 as imageio

def load_nii_gz(path):
    """Load a .nii.gz file as a NumPy array."""
    nii = nib.load(path)
    volume = nii.get_fdata()
    return volume

def save_slices(volume, output_dir, fmt='png'):
    """Save volume slices as 2D images in the specified format."""
    os.makedirs(output_dir, exist_ok=True)
    volume = (volume - np.min(volume)) / (np.max(volume) - np.min(volume))  # normalize to 0-1

    for i in range(volume.shape[2]):
        slice_2d = (volume[:, :, i] * 255).astype(np.uint8)
        filename = os.path.join(output_dir, f'slice_{i:03d}.{fmt}')
        imageio.imwrite(filename, slice_2d)

    print(f"Saved {volume.shape[2]} slices in '{output_dir}'.")

def main():
    parser = argparse.ArgumentParser(description="Load a .nii.gz file and optionally save its slices as images.")
    parser.add_argument("input", help="Path to the .nii.gz file")
    parser.add_argument("--save", metavar="FORMAT", help="Image format to save slices (e.g., png, jpg)")
    parser.add_argument("--outdir", default="output_slices", help="Output directory for saved images")

    args = parser.parse_args()

    volume = load_nii_gz(args.input)
    print(f"Volume loaded: shape = {volume.shape}, dtype = {volume.dtype}")

    if args.save:
        save_slices(volume, args.outdir, fmt=args.save)

if __name__ == "__main__":
    main()
