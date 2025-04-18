# Artifact evaluation

This document describes how to reproduce the results presented in the paper *"Soaring with TRILLI: an HW/SW Heterogeneous Accelerator for Multi-Modal Image Registration"*, submitted to the 33rd IEEE International Symposium on Field-Programmable Custom Computing Machines (FCCM 2025).
Specifically, we provide instructions on how to compile the host applications and run the experiments using the prepared bitstreams to reproduce the results presented in the paper.

The figures that can be reproduced are the following:
- [Figure 6](#figure-6-geometric-transformation-ipe-scaling) - Geometric transformation IPE scaling
- [Figure 7](#figure-7-transformation-mi-and-complete-registration-comparison-with-soa) - Transformation, MI and Complete Registration comparison with SoA
- [Figure 8](#figure-8-registration-accuracy) - Registration accuracy

## Abstract

3D rigid image registration is a pivotal procedure in computer vision that aligns a floating volume with a reference one to correct positional and rotational distortions. It serves either as a stand-alone process or as a pre-processing step for non-rigid registration, where the rigid part dominates the computational cost. Various hardware accelerators have been proposed to optimize its compute-intensive components: geometric transformation with interpolation and similarity metric computation. However, existing solutions fail to address both components effectively, as GPUs excel at image transformation, while FPGAs in similarity metric computation. To close this gap, we propose TRILLI, a novel Versal-based accelerator for image transformation and interpolation. TRILLI optimally maps each computational step on the proper heterogeneous hardware component. TRILLI achieves between 5.32× and 36.75× speedup and between 10.04× and 104.60× energy efficiency improvement for image transformation and interpolation against the top hardware accelerated solutions. Moreover, we integrate it with an FPGA-based similarity metric from literature to complete a rigid image registration step (i.e., transformation, interpolation, and similarity metric) attaining between 18.60× and 74.04× speedup and between 36.11× and 117.65× energy efficiency improvement over the top-performing hardware-accelerated solutions.

## Requirements

***Software Dependencies***
- *Vitis 2022.1 & Vivado 2022.1*: To build the different designs
- *XRT 2022.1*:  To target the accelerator
- *OpenCV-3.0.0 - Static Library*: To load and store images
- *Python 3.8*
- *GCC 7.3.1*
- *OS: CentOS Linux release 7.9.2009 (Core)*
  
*Note: [NOT TESTED] As we do not use any OS-specific feature, different Linux-based OS versions may work as well.*


***Hardware Dependencies***
- *Versal VCK5000 - XDMA2022.1* 
. *PCIe 3.0* 
- *Intel I7-4470*: Other CPUs may work as well, but currently untested.

## Experiments workflow

The folder `bitstreams/` contains the bitstreams used for the evaluation. 
Alternatively, the bitstreams can be rebuilt by following the instructions in the [building](./README.md#building) section.

### Preliminary steps
1. Clone the repository
    ```bash
    git clone https://github.com/necst/trilli.git
    ```
2. Move into the repository
    ```bash
    cd trilli
    ```
3. Source Vitis & XRT
    ```bash
    source <YOUR_PATH_TO_XRT>/setup.sh
    source <YOUR_PATH_TO_VITIS>/2022.1/settings64.sh
    ```
4. Build the host code for all the necessary configurations:
    ```bash
    ./build_hosts.sh
    ```
    This will create multiple folders under `build/` containing the various configurations that should be tested.
5. Move the `build/` folder to the deploy machine

---

### Figure 6. Geometric transformation IPE scaling
***Note: the following operations must be performed on the deploy machine (where the `build/` folder has been moved).***

In figure 6, we evaluate how scaling the number of IPEs (1, 2, 4, 8, 16 and 32) impacts execution time for the geometric transformation with interpolation, for different depths (32, 64, 128, 256 and 512). The builds for this experiment are placed in subfolders under the `build/` folder, named `onlyTX_XXIPE`, where `XX` is the number of IPEs. The needed builds are the following:
- `build/onlyTX_01IPE`
- `build/onlyTX_02IPE`
- `build/onlyTX_04IPE`
- `build/onlyTX_08IPE`
- `build/onlyTX_16IPE`
- `build/onlyTX_32IPE`

#### Flow

1. Move into `build/` and source XRT:
    ```bash
    cd build
    source <YOUR_PATH_TO_XRT>/setup.sh
    ```
2. For each configuration, enter its respective folder under `build/`. E.g.
    ```bash
    cd onlyTX_01IPE
    ```
3. Run the experiment:
    ```bash
    ./run_scaling_depth.sh
    ```
    This will run the transformation for each depth (32, 64, 128, 256 and 512) and store the execution times in 5 different csv files.
4. After all configurations have been run, each folder will contain 5 csv files, one for each depth. For plotting, the csv files need to be copied into `paper_fig/figure6/csv/`. To do so, launch the following command in the `build/` folder:
    ```bash
    cd ..
    ./gather_results_fig6.sh
    ```
5. Plot figure 6:
    ```bash
    cd paper_fig/figure6/
    python3 figure6.py
    ```

----

### Figure 7. Transformation, MI and Complete Registration comparison with SoA
***Note: the following operations must be performed on the deploy machine (where the `build/` folder has been moved).***

In figure 7, we compare the execution times of the transformation only (`build/onlyTX_32IPE`), single registration step (`build/STEP_32IPE`) and complete registration application (`build/3DIR_Application`) against the state of the art.

#### Flow

1. Move into `build/` if you haven't done it before, and source XRT:
    ```bash
    cd build
    source <YOUR_PATH_TO_XRT>/setup.sh
    ```
2. If you have already run the configurations for Figure 6, configuration `onlyTX_32IPE` has already been ru and you can skip to the next step. Otherwise, enter the folder and run the tests:
    ```bash
    cd onlyTX_32IPE
    ./run_for_SoA_comparison.sh
    cd ..
    ```
3. For the single registration step, enter the respective folder and run the tests:
    ```bash
    cd STEP_32IPE
    ./run_for_SoA_comparison.sh
    cd ..
    ```
4. Finally, run the complete registration application:
    ```bash
    cd 3DIR_Application
    ./exec.sh
    ```
    ***Note: to get a proper dataset contact the authors privately. Alternatively, run*** `./generate_dataset.sh`
5. Each folder will contain a csv files with the execution times. For plotting, the csv files need to be copied into `paper_fig/figure7/csv/`. To do so, launch the following command in the `build/` folder:
    ```bash
    cd ..
    ./gather_results_fig7.sh
    ```
6. Plot figure 7:
    ```bash
    cd paper_fig/figure7/
    python3 figure7.py
    ```

----

### Figure 8. Registration accuracy
***Note: the following operations must be performed on the deploy machine (where the `build/` folder has been moved).***

This figure evaluate the registration correctness upon the whole 3D image registration step, to align a transformed floating volume with respect to a reference. 

The paper_fig/figure8/data already contains all the material for reproducing the figure. 
Alternatively, the images in data folder needs to be re-created.
In this latter case, please contact us and we will **privately** send you the dataset.

#### Option 1: Use provided data

```bash
cd build/paper_fig/figure8/
python3 figure8.py
```
#### Option 2: Reproduce all the images from scratch

1. Move into `build/` if you haven't done it before, and source XRT:
    ```bash
    cd build
    source <YOUR_PATH_TO_XRT>/setup.sh
    ```
2. Apply a deformation to the floating volume:
    ```bash
    ./generate_distortion.sh 246 10 10 10
    ```
3. Now, apply the 3D image registration step using the distorted volume as floating volume:
    ```bash
    cd 3DIRG_Application
    ./exec.sh ../onlyTX_32IPE/dataset_output/
    cd ..
4. To plot this figure, the first slice from the original volume, from the distorted one and from the registered one, need to be copied into `paper_fig/figure8/data/`. To do so, launch the following command in the `build/` folder:
    ```bash
    ./gather_results_fig8.sh
    ```
5. Now it is possible to produce the plot again
    ```bash
    cd paper_fig/figure8/
    python3 figure8.py
    ```
