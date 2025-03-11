# 📊 Figure 7 - Execution Time Literature Comparison
This folder contains the necessary files for plotting **Figure 7**. 

> **Important:** Ensure that LaTeX fonts are enabled in Python to guarantee correct image rendering.

## 📌 Overview
Figure 7 presents a execution time comparison against literature solutions accelerated on FPGAs, CPUs or GPUs. We evaluate standalone
image transformation and interpolation (a), its integration with MI (b), and the complete 3D image registration application (c)

## 🚀 How to Run the Script
To execute the code, simply run:
```
python3 figure7.py
```
As default, this python runs looking for the files we provided, containing results for TRILLI registration step and full 3D image registration application.

## 🔄 Customizing the Plot with Your Data

If you want to modify the plot using **your own generated data** for TRILLI, follow these steps:

1. Refer to the chosen section in the **README** (located in the root folder). Follow the section to produce the bitstream and run new data.
2. Run the following command
```
python3 figure7.py --trilli_tx <PATH_TO_RESULTS_FILE_FOR_RIGIDSTEP> --trilli_pow <PATH_TO_RESULTS_FILE_FOR_FULL3DAPP>
```


