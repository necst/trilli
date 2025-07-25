import argparse
import os
import numpy as np
import nibabel as nib
import imageio.v2 as imageio

def to_resolution(volume: np.ndarray, target_width: int, target_height: int) -> np.ndarray:
    """
    Padded a 3D volume with zeros so each 2D slice has resolution (target_width x target_height).

    Args:
        volume (np.ndarray): 3D input volume of shape (width, height, depth)
        target_width (int): desired width of each slice
        target_height (int): desired height of each slice

    Returns:
        np.ndarray: padded volume with shape (target_width, target_height, depth)
    """
    current_width, current_height, depth = volume.shape

    pad_x = target_width - current_width
    pad_y = target_height - current_height

    if pad_x < 0 or pad_y < 0:
        raise ValueError(f"Target resolution ({target_width}x{target_height}) must be >= current ({current_width}x{current_height})")

    pad_x_before = pad_x // 2
    pad_x_after = pad_x - pad_x_before
    pad_y_before = pad_y // 2
    pad_y_after = pad_y - pad_y_before

    padded_volume = np.pad(
        volume,
        pad_width=((pad_x_before, pad_x_after),
                   (pad_y_before, pad_y_after),
                   (0, 0)),  # No padding along depth
        mode='edge'    )

    return padded_volume

def load_nii_gz(path, dtype=np.uint8):
    """Load a .nii.gz file and cast it to the specified dtype (default: uint8)."""
    nii = nib.load(path)
    volume = nii.get_fdata().astype(dtype)
    return volume

def save_slices(volume, output_dir, fmt='png', dtype=np.uint8):
    """Save volume slices as 2D images in the specified format and dtype."""
    os.makedirs(output_dir, exist_ok=True)

    if dtype == np.uint8 and volume.max() <= 1.0:
        # Assume input is normalized float [0,1]
        volume = (volume * 255).astype(np.uint8)
    else:
        volume = volume.astype(dtype)

    for i in range(volume.shape[2]):
        slice_2d = volume[:, :, i]
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
