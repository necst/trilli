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
3. Run the tests for the different depths (32, 64, 128, 256 and 512) by executing the following command:
    ```bash
    ./run_scaling_depth.sh
    ```
4. Each folder will contain 5 csv files, one for each depth. For plotting, the csv files need to be copied into `paper_fig/figure6/csv/`. To do so, launch the following command in the `build/` folder:
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
    ./gather_images_fig8.sh
    ```
5. Now it is possible to produce the plot again
    ```bash
    cd paper_fig/figure8/
    python3 figure8.py
    ```
