#!/bin/bash

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

# /******************************************
# *MIT License
# *
# *Copyright (c) [2021] [Eleonora D'Arnese, Emanuele Del Sozzo, Davide Conficconi,  Marco Domenico Santambrogio]
# *
# *Permission is hereby granted, free of charge, to any person obtaining a copy
# *of this software and associated documentation files (the "Software"), to deal
# *in the Software without restriction, including without limitation the rights
# *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# *copies of the Software, and to permit persons to whom the Software is
# *furnished to do so, subject to the following conditions:
# *
# *The above copyright notice and this permission notice shall be included in all
# *copies or substantial portions of the Software.
# *
# *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# *SOFTWARE.
# ******************************************/

           
IMG_DIM=512
PYCODE_onestep=single_step.py
PYCODE_Powell=powell_torch.py
PYCODE_oneplusone=one_plus_one_torch.py
PYCODE_oneplusone_mod=one_plus_one_modified.py
PYCODE_oneplusone_Heph=one_plus_one.py

DATASET_FLDR=./
CT_PATH=Dataset/reference
PET_PATH=Dataset/reference
GOLDEN_PATH=Dataset/reference
RES_PATH=./output/
metric=(MI)

dev=(cuda:0)
#dev=(cpu)
for i in "${metric[@]}"
do
    for j in "${dev[@]}"
    do
        for i in {1..1}
        do
          echo "python $PYCODE_onestep -cp $CT_PATH -pp $PET_PATH -rp $RES_PATH/singleStep_${i}_${j} -t 1 -px $DATASET_FLDR -im $IMG_DIM -dvc $j -vol 246 -f fccm.csv"
          python3 $PYCODE_onestep -cp $CT_PATH -pp $PET_PATH -rp $RES_PATH -t 1 -px $DATASET_FLDR -im $IMG_DIM -dvc $j -vol 246 --mode bilinear -f bilinear_512.csv
        done
    done
done
