# MIT License

# Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import numpy as np
import sys
import cv2
import os

# python3 image_generator.py 64 64 sw/dataset_BW

if __name__ == '__main__':
    # take as arguments from sys argv: size, path where to store the images
    
    if len(sys.argv) != 4:
        print("Usage: python image_generator.py <size> <n_couples> <path>")
        exit(1)
    
    size = int(sys.argv[1])
    n_couples = int(sys.argv[2]) # depth
    path = sys.argv[3]

    if path[-1] != "/":
        path += "/"
    
    REP = 32

    volume = np.zeros((n_couples, size, size), dtype=np.uint8)
    for i in range(n_couples):
        #volume[i,:,:] = 255 * (i % REP) / (REP - 1 if REP <= n_couples else n_couples - 1)
        volume[i,:,:] = 100 if i % REP < (REP / 2) else 255
        if i >= 32:
            volume[i,:,:] = 50



    # if path does not exist, create it
    if not os.path.exists(path):
        os.makedirs(path)

    # save the volume as a .png file with opencv
    for i in range(n_couples):
        cv2.imwrite(path + "IM" + str(i+1) + ".png", volume[i,:,:])
    


