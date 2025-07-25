import argparse
import os
import numpy as np
import nibabel as nib
import imageio.v2 as imageio

import os
import numpy as np
import imageio.v2 as imageio

def read_volume_with_datalayout_from_array(volume, border_padding=0, depth_padding=0):
    """
    Same as read_volume_with_datalayout, but from numpy 3D array instead of images.

    Args:
        volume (np.ndarray): 3D array (width, height, depth)
        border_padding (int): padding on x/y
        depth_padding (int): number of zero slices to add

    Returns:
        flat_buffer, padded_size, total_depth
    """
    size_x, size_y, depth = volume.shape
    if size_x != size_y:
        raise ValueError("Only square slices supported for now")
    size = size_x

    padded_size = size + 2 * border_padding
    total_depth = depth + depth_padding
    slice_area = padded_size * padded_size
    flat_buffer = np.zeros(total_depth * slice_area, dtype=np.uint8)

    for z in range(depth):
        slice_2d = volume[:, :, z]
        if border_padding > 0:
            slice_2d = np.pad(slice_2d, ((border_padding, border_padding), (border_padding, border_padding)), mode='constant', constant_values=0)
        flat_slice = slice_2d.flatten(order='C')
        for i in range(slice_area):
            flat_buffer[i * total_depth + z] = flat_slice[i]

    return flat_buffer, padded_size, total_depth

def read_volume_with_datalayout(path, size, n_couples, border_padding, depth_padding):
    """
    Reads PNG slices and flattens them into a 1D buffer using interleaved data layout:
    dest[i * total_depth + z] = pixel_i from slice z

    Args:
        path (str): Directory containing IM0.png, IM1.png, ...
        size (int): Original image width/height (no padding)
        n_couples (int): Number of slices to load
        border_padding (int): Padding to apply around each slice
        depth_padding (int): Additional zero slices to append

    Returns:
        flat_buffer (np.ndarray): 1D uint8 array of shape (W * H * (n_couples + depth_padding))
        padded_size (int): Final width/height after padding
        total_depth (int): n_couples + depth_padding
    """
    padded_size = size + 2 * border_padding
    total_depth = n_couples + depth_padding
    slice_area = padded_size * padded_size

    flat_buffer = np.zeros(total_depth * slice_area, dtype=np.uint8)

    for z in range(n_couples):
        file_path = os.path.join(path, f"IM{z}.png")
        img = imageio.imread(file_path)

        if img is None or img.ndim != 2:
            raise FileNotFoundError(f"Image not found or invalid: {file_path}")
        if img.shape != (size, size):
            raise ValueError(f"Unexpected image shape: {img.shape}, expected ({size}, {size})")
        # Apply border padding
        if border_padding > 0:
            img = np.pad(img, ((border_padding, border_padding), (border_padding, border_padding)), mode='constant', constant_values=0)

        img_flat = img.T.flatten(order='C')

        for i in range(slice_area):
            flat_buffer[i * total_depth + z] = img_flat[i]

    # Depth padding is already zero-initialized
    return flat_buffer, padded_size, total_depth

def write_volume_with_datalayout(flat_buffer, width, height, depth, output_dir):
    """
    Writes volume slices (PNG) from a flat buffer using interleaved layout:
    pixel = buffer[i * depth + z]

    Args:
        flat_buffer (np.ndarray): 1D uint8 buffer in interleaved layout
        width (int): Width of each 2D slice
        height (int): Height of each 2D slice
        depth (int): Number of slices to write
        output_dir (str): Destination folder
    """
    import os
    os.makedirs(output_dir, exist_ok=True)

    slice_area = width * height

    for z in range(depth):
        slice_flat = np.empty((slice_area,), dtype=np.uint8)

        for i in range(slice_area):
            slice_flat[i] = flat_buffer[i * depth + z]

        slice_2d = slice_flat.reshape((width, height), order='C').T
        out_path = os.path.join(output_dir, f"IM{z}.png")
        imageio.imwrite(out_path, slice_2d)

    print(f"Saved {depth} slices to '{output_dir}'")

def volume_to_flat_slices(volume: np.ndarray, depth_padding=0) -> list:
    """
    Converts a 3D volume (shape: W x H x D) to a flat list with the depth-wise layout
    """
    W, H, D = volume.shape
    flat_data = []
    for z in range(D):
        slice_2d = volume[:, :, z]
        flat_data.extend(slice_2d.flatten(order='C')) 
    # Add padding slices (zero-filled)
    for _ in range(depth_padding):
        flat_data.extend([0] * (W * H))
    return flat_data

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

def load_nii_gz(path, dtype=np.uint8, rescale_to_uint8=True):
    """
    Load a .nii(.gz) file and optionally rescale to uint8.

    Args:
        path (str): Path to the NIfTI file.
        dtype (np.dtype): Desired output type (default: np.uint8).
        rescale_to_uint8 (bool): If True and dtype == uint8, rescales intensities to [0,255].

    Returns:
        np.ndarray: 3D volume array.
    """
    nii = nib.load(path)
    volume = nii.get_fdata()

    if dtype == np.uint8 and rescale_to_uint8:
        min_val = np.min(volume)
        max_val = np.max(volume)
        if max_val - min_val < 1e-6:
            # Avoid divide-by-zero if image is flat
            volume = np.zeros_like(volume, dtype=np.uint8)
        else:
            if min_val < 0 or max_val > 255:
                # Rescale only if values are outside [0, 255]
                print(f"ALERT - Pixels are not normalized in 8bits, MIN={min_val} - MAX{max_val} -> goint to rescale to UINT8.")
                volume = (volume - min_val) / (max_val - min_val)
                volume = (volume * 255).astype(np.uint8)
    else:
        volume = volume.astype(dtype)

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
