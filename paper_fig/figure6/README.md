# 📊 Figure 6 - Latency Improvement Analysis

This folder contains the necessary files for plotting **Figure 6**. 

> **Important:** Ensure that LaTeX fonts are enabled in Python to guarantee correct image rendering.

## 📌 Overview
Figure 6 presents a **latency improvement analysis** for *image transformation*, focusing on **scaling IPEs** and targeting **3D volumes** of **512 × 512 images** with variable depths.  
Additionally, this script also generates plots for **resolutions 32×32 and 128×128**, maintaining the same volume depths.

## 🚀 How to Run the Script
To execute the code, simply run:

```
python3 figure6.py
```

## 🔄 Customizing the Plot with Your Data

If you want to modify the plot using **your own generated data**, follow these steps:

1. Refer to the **Image Transformation Test** section in the **README** (located in the root folder).
2. Rename your output file according to the target configuration.
3. Replace it in the **CSV folder** to ensure proper processing.

### 📂 File Naming Format:
Each CSV file should be named as:

```plaintext
time_IPEX_DYY_NZZ.csv
```

In which X represents the number of IPE, YY represents the target volume resolution (e.g., 512 for 512x512 images), N represents the target volume depth.

