# Soaring with TRILLI: an HW/SW Heterogeneous Accelerator for Multi-Modal Image Registration

TRILLI is a novel Versal-based accelerator for **3D rigid image registration**.
TRILLI is designed to address the computational challenges in both key components of the registration process, **geometric transformation with interpolation** and **similarity metric** computation, by optimally mapping computational steps to heterogeneous hardware components on the [Versal VCK5000](https://japan.xilinx.com/content/dam/xilinx/publications/product-briefs/amd-xilinx-vck5000-product-brief.pdf).

## System architecture
![System Architecture](./figures/architecture_diagram.png)

*System Architecture Diagram: TRILLI integration with a CPU-based Powell optimizer for multi-modal 3D rigid image registration. Input images are used for an initial transformation and accelerated registration via TRILLI. The resulting MI is used by the Powell optimizer
to iteratively refine transformation parameters based on user-defined settings. The final output is a registered floating volume.*

## Requirements
- Hardware Device: Versal VCK5000 XDMA2022.1
- Vitis 2022.1 
- XRT 2022.1
- OpenCV 3.0.0 - static library
- Python 3.8

## Code overview
- `3DIRG_application/`: complete ragistration framework
- `aie/`: AI Engines source code
- `common/`: constants and configuration generator
- `data_movers/`: PL kernels source code
- `figures/`: figure for TRILLI's repository
- `hw/`: system integration and output bitstream
- `mutual_info/`: PL mutual information kernel source code from **[Hephaestus](https://dl.acm.org/doi/10.1145/3607928)**
- `soa/`: GPU 3D Image Registration from athena, with scripts for simplifying testing
- `sw/`: host source code
- `default.cfg`: architecutre configuration parameters

## Artifact evaluation
Following we describe three testing flows: 

- Case 1: using the given bitstream to test image transformation and/or image registration step
- Case 2: building from scratch the desired bitstream for image transformation and/or image registration step
- Case 3: target the whole image registration application. This can be done either using the given bitstream for image registration step, or building it from scratch

### Case 1. Using given bitstreams for Image Transformation or Image Registration Step
1. Choose a bitstream in `bitstreams/only_TX` or `bitstreams/reg_step` and copy it into `hw/`
2. Compile the host code: `make build_sw TASK=[TX|STEP][TX|STEP]`
3. Pack the build into a single folder, ready for testing: `make pack [NAME=<name>]` (default name is `hw_build`). This command generates a folder NAME under build/. To use the given bitstream, it must be copied into this folder.
4. Move the generated folder, `build/NAME` (i.e. `cd build/hw_build`), on the deploy machine
5. Run: `./host_overlay.exe [depth] [x] [y] [ang_degrees] [num_runs]`

### Case 2. Preparing the bitstream for Image Transformation or Rigid Registration Step

1. Source Vitis & XRT
```
source <YOUR_PATH_TO_XRT>/setup.sh
source <YOUR_PATH_TO_VITIS>/2022.1/settings64.sh
```
2. Move into the root folder of this repository & build the transformation standalone bitstream
```
cd <YourPath>/Trilli
```
3. Edit the default.cfg file to detail the configuration desired. 

*For Transformation, relevant parameters are:*
 - DIMENSION := XYZ // represents the image resolution DIMENSION x DIMENSION. Choices = [1,2,4,8,16]
 - INT_PE := XY // Number of Interpolation Processing Elements. Choices: [1,2,4,8,16,32]
 - PIXELS_PER_READ := XYZ // represents the port width. [32,64,128]

*For the rigid step, instead:*
 - DIMENSION := XY // represents the image resolution DIMENSION x DIMENSION, 512 for the paper
 - HIST_PE := XY // Histogram Processing elements for Mutual Information. Choices = [1,2,4,8,16]
 - EPE_PE := XY // Histogram Processing elements for Mutual Information. Choices = [1,2,4,8,16]
 - INT_PE := XY // Number of Interpolation Processing Elements. Choices: [1,2,4,8,16,32]
 - PIXELS_PER_READ := XYZ // represents the port width. Choices: [32,64,128]

4. Prepare the folder to be moved on the deploy machine. (default name is `hw_build`)
```
make build_and_pack TARGET=hw TASK=[TX|STEP] NAME=[NAME=<name>]
```
5. Move the generated folder, `build/NAME` (i.e. `cd build/hw_build`), on the deploy machine

6. Source XRT to interact with the device
```
source <YOUR_PATH_TO_XRT>/setup.sh
```
7. Run The following command:
```
./host_overlay.exe [depth] [x] [y] [ang_degrees] [num_runs]
```

### Case 3. Complete image registration application 
#### Option A. Using pre-generated bitstream

1. Compile the software application. We remind the hard requirements of OpenCV 3.0.0 installed and statically compiled.
```
make build_app
```
2. Move the CT volume in `3DIRG_application/PET_small/png` and the PET volume in `3DIRG_application/CT_small/png`
3. Prepare the folder. (default name is `hw_build`)
```
make pack_app [NAME=<name>]
```
*Note 1:* That the commands prepare a folder copying dataset volumes from `3DIRG_application/CT_small/png` and `3DIRG_application/PET_small/png`. Therefore, images must be there before using this command. 

*Note 2:* This command would copy a newly generated bitstream. To use the premade one, you need to manually copy it in the generated folder.
4. Move the folder on the deploy machine
5. Execute the application: 
```
./exec.sh 
```

#### Option B. Preparing bitstream and build the application

1. Follow CASE 1, selecting STEP as TASK
2. Compile the software application. We remind the hard requirements of OpenCV 3.0.0 installed and statically compiled.
```
make build_app
```
3. Move the CT volume in `3DIRG_application/PET_small/png` and the PET volume in `3DIRG_application/CT_small/png`
4. Prepare the folder. (default name is `hw_build`)
```
make pack_app [NAME=<name>]
```
*Note 1:* That the commands prepare a folder copying dataset volumes from `3DIRG_application/CT_small/png` and `3DIRG_application/PET_small/png`. Therefore, images must be there before using this command. 

5. Execute the application: 
```
./exec.sh 
```

## Plotting Paper Figures
To plot each result figure in the paper, please refer to the corresponding folder under **paper_fig/.**
Each folder contains a subfolder with the figure name, and a dedicated readme for running. Per each figure, we provide some dedicated .csv files, containing sufficient numbers to replicate the paper result.


## Related Publications/Repository for Literature Comparison

- **[Hephaestus](https://dl.acm.org/doi/10.1145/3607928)** - Mutual Information & CPU-FPGA 3D image registration 
- **[Athena](https://doi.org/10.1109/BioCAS58349.2023.10388589)** - GPU-based 3D image registration
- **[Vitis Libraries](https://github.com/Xilinx/Vitis_Libraries/tree/2022.1)** - WarpAffine3D kernel for image transformation
- **[ITK](https://github.com/InsightSoftwareConsortium/ITK/)** powell-based 3D image registration
- **[SimpleITK](https://github.com/SimpleITK/SimpleITK)** powell-based 3D image registration
