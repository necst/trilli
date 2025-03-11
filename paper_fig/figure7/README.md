# 📊 Figure 7 - Latency Improvement Analysis

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

## 🔄 Customizing the Plot with Your Data

If you want to modify the plot using **your own generated data**, follow these steps:

1. Refer to the chosen section in the **README** (located in the root folder). Follow the section to produce the bitstream and run new data.
2. Rename your output file according to the target configuration.
3. Replace it in the **CSV folder** to ensure proper processing.

### 📂 File Naming Format:
As the script looks for specific naming, please keep it when moving in the folder. Alternatively, you can update the script
