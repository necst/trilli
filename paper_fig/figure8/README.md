# 📊 Figure 8 - Registration Error Visualization

This folder contains the necessary files for plotting **Figure 8**. 

> **Important:** Ensure that LaTeX fonts are enabled in Python to guarantee correct image rendering.

## 📌 Overview
Figure 8 visualizes the error upon image registration, taking as reference a single slice. We show the initial image, the transformed one, that must be realinged, the ideal golden model and the difference between the real output and the ideal one. As the error is mostly invisible on the full picture, we highlight it to clarify the *amount* of wrong pixels. 

## 🚀 How to Run the Script
To execute the code, simply run:

```
python3 figure8.py
```

## 🔄 Customizing the Plot with Your Data

If you want to modify the plot using **your own generated data**, consider reproducing the whole 3D image registration application, and put in the *data/* folder the gold_image, float_image and result_image. 

- gold_image: ideally registered image
- float_image: starting image showing the deformation with respect to the initial image
- result_image: showing your registered image upon the image registration

### 📂 File Naming Format:
As the script looks for specific naming, please keep it when moving in the folder. Alternatively, you can update the script
