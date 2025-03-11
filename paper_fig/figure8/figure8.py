""""
"MIT License

Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import cv2
import matplotlib.pyplot as plt
import numpy as np

plt.rcdefaults()
plt.rcParams.update({
    "text.usetex": True,
    "font.family": "serif",
    "font.serif": ["Palatino"],
})
plt.rcParams["font.size"] = 12
plt.rcParams["xtick.labelsize"] = 12
plt.rcParams["ytick.labelsize"] = 12
plt.rcParams["legend.fontsize"] = 12
plt.rcParams["legend.handletextpad"] = 0.01
plt.rcParams['hatch.linewidth'] = 0.6
plt.rcParams['axes.labelpad'] = 0
plt.rcParams['pdf.fonttype'] = 42
plt.rcParams['ps.fonttype'] = 42

def custom_threshold(image, threshold, max_value):
    """Applies a custom threshold to a grayscale image."""
    return np.where(image < threshold, 0, max_value).astype(np.uint8)

def display_images_with_difference(path_img1, path_img2, path_img3):
    """Displays three images with a computed registration error."""

    # Load grayscale images
    img1 = cv2.imread(path_img1, cv2.IMREAD_GRAYSCALE)
    img2 = cv2.imread(path_img2, cv2.IMREAD_GRAYSCALE)
    img3 = cv2.imread(path_img3, cv2.IMREAD_GRAYSCALE)

    if img1 is None or img2 is None or img3 is None:
        print("Error loading images. Check file paths.")
        return

    # Apply thresholding
    THRESHOLD = 30
    MAX_VALUE = 255
    binary_img1 = custom_threshold(img1, THRESHOLD, MAX_VALUE)
    binary_img3 = custom_threshold(img3, THRESHOLD, MAX_VALUE)

    # Compute the difference between thresholded images
    diff_img = cv2.bitwise_xor(binary_img1, binary_img3)

    # Highlight differences in red
    colored_diff = np.ones((diff_img.shape[0], diff_img.shape[1], 3), dtype=np.uint8) * 255
    colored_diff[diff_img == 255] = [255, 0, 0]

    # Add black border around the difference
    bordered_diff = cv2.copyMakeBorder(colored_diff, 2, 2, 2, 2, cv2.BORDER_CONSTANT, value=[0, 0, 0])

    # Display images in a 1x4 grid
    plt.figure(figsize=(12, 10))

    # Gold Image
    ax1 = plt.subplot(1, 4, 1)
    ax1.imshow(img1, cmap='gray')
    ax1.set_title('Gold Image', fontsize=18)
    ax1.set_xticks([0, 128, 256, 384, 512])
    ax1.set_yticks([0, 128, 256, 384, 512])
    ax1.set_yticklabels(ax1.get_yticks(), fontsize=14)
    ax1.set_xticklabels(ax1.get_xticks(), fontsize=14)
    ax1.axis('on')

    # Image Upon Distortion
    ax2 = plt.subplot(1, 4, 2)
    ax2.imshow(img2, cmap='gray')
    ax2.set_title('Image Upon Distortion', fontsize=18)
    ax2.set_xticks([0, 128, 256, 384, 512])
    ax2.set_xticklabels(ax2.get_xticks(), fontsize=14)
    ax2.get_yaxis().set_visible(False)
    ax2.axis('on')

    # Image Upon Registration
    ax3 = plt.subplot(1, 4, 3)
    ax3.imshow(img3, cmap='gray')
    ax3.set_title('Image Upon Registration', fontsize=18)
    ax3.set_xticks([0, 128, 256, 384, 512])
    ax3.set_xticklabels(ax3.get_xticks(), fontsize=14)
    ax3.get_yaxis().set_visible(False)
    ax3.axis('on')

    # Registration Error
    ax4 = plt.subplot(1, 4, 4)
    ax4.imshow(bordered_diff)
    ax4.set_title('Registration Error', fontsize=18)
    ax4.set_xticks([0, 128, 256, 384, 512])
    ax4.set_xticklabels(ax4.get_xticks(), fontsize=14)
    ax4.get_yaxis().set_visible(False)
    ax4.axis('on')

    # Save figure
    plt.tight_layout()
    plt.savefig("image_comparison_registration_error_with_border_and_axes.pdf", dpi=300, bbox_inches='tight')


# File paths
path_img1 = "./data/gold_IM0.png"
path_img2 = "./data/float_IM0.png"
path_img3 = "./data/result_IM0.png"

# Execute function
display_images_with_difference(path_img1, path_img2, path_img3)
