# Artifact evaluation

## Abstract

***TODO write abstract***

## Requirements

***TODO write info about sw and hw dependencies***

## Testing

The folder `bitstreams/` contains the bitstreams used for the evaluation. If, for any reason, the bitstreams need to be rebuilt, follow the steps in ... ***TODO link to other README with build instructions***

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
5. Finish packing the builds into the `build/` folder by running:
    ```bash
    ./finish_packing.sh
    ```
6. Move the `build/` folder to the deploy machine

---

### Figure 6. Geometric transformation IPE scaling
***Note: the following operations must be performed on the deploy machine (where the `build/` folder has been moved).***

***TODO add brief description of the plot (?)*** 

The builds for testing the scaling of the IPEs (1, 2, 4, 8, 16 and 32) for the geometric transformation are placed in the `build/` folder. The folder contains multiple subfolders, each corresponding to a different number of IPEs. Such folders are:
- `build/onlyTX_01IPE`
- `build/onlyTX_02IPE`
- `build/onlyTX_04IPE`
- `build/onlyTX_08IPE`
- `build/onlyTX_16IPE`
- `build/onlyTX_32IPE`

1. Source XRT
    ```bash
    source <YOUR_PATH_TO_XRT>/setup.sh
    ```
2. For each configuration, enter the respective folder. E.g.
    ```bash
    cd build/onlyTX_01IPE
    ```
3. Run the tests for the different depths (32, 64, 128, 256 and 512) by executing the following command:
    ```bash
    ./run_scaling_depth.sh
    ```
4. Each folder will contain 5 csv files, one for each depth. For plotting, the csv files need to be copied into `paper_fig/figure6/csv/`. To do so, launch the following command in the `build/` folder:
    ```bash
    cd ..
    ./gather_results_fig6.sh
    ```
5. ***TODO complete the procedure for plotting***

----

### Figure 7. Transformation, MI and Complete Registration comparison with SoA
***Note: the following operations must be performed on the deploy machine (where the `build/` folder has been moved).***

In figure 7, we compare the execution times of the transformation only (`build/onlyTX_32IPE`), single registration step (`build/STEP_32IPE`) and complete registration application (`build/3DIR_Application`) against the state of the art.

1. Source XRT
    ```bash
    source <YOUR_PATH_TO_XRT>/setup.sh
    ```
2. If you have already run the configurations for Figure 6, you can skip running `build/onlyTX_32IPE`. Otherwise, enter the folder and run the tests:
    ```bash
    cd build/onlyTX_32IPE
    ./run_for_SoA_comparison.sh
    cd -
    ```
3. For the single registration step, enter the respective folder and run the tests:
    ```bash
    cd build/STEP_32IPE
    ./run_for_SoA_comparison.sh
    cd -
    ```
4. Finally, run the complete registration application:
    ```bash
    cd build/3DIR_Application
    ./exec.sh
    ```
    ***Note: to get a proper dataset contact the authors privately. Alternatively, run*** `./generate_dataset.sh`
5. Each folder will contain a csv files with the execution times. For plotting, the csv files need to be copied into `paper_fig/figure7/csv/`. To do so, launch the following command in the `build/` folder:
    ```bash
    cd ..
    ./gather_results_fig7.sh
    ```
6. ***TODO complete the procedure for plotting***

### Figure 8. Registration accuracy

This figure evaluate the registration correctness upon the whole 3D image registration step, to align a transformed floating volume with respect to a reference. 

The paper_fig/figure8/data already contains all the material for reproducing the figure. 
Alternatively, the images in data folder needs to be re-created.
In this latter case, please contact us and we will **privately** send you the dataset.

Note: build/figure8steps contains 2 different builds. One is a simple transformation step, to apply deformation. The other is for 3D image registration application

#### Option 1: Use provided data

```bash
cd build/paper_fig/figure8/
python3 figure8.py
```
#### Option 2: Reproduce all the images from scratch

1. Take the floating volume and apply a deformation using TRILLI TX step
    ```bash
    cd build/figure8steps/realvolume_onlyTX_32IPE
    ./execute_and_prepare 246 10 10 10 1
    ```
3. At this point, it is time to register the deformed image to correct the applied deformation
    ```bash
        cd ../3DIRG_app_build/
        ./exec.sh
        ./gather_images_figure8.sh
    ```
4. Now it is possible to produce the plot again
    ```bash
        cd ../../paper_fig/figure8/
        python3 figure8.py
    ```