# Soaring with TRILLI: an HW/SW Heterogeneous Accelerator for Multi-Modal Image Registration

TRILLI is a novel Versal-based accelerator for **3D rigid image registration**.
TRILLI is designed to address the computational challenges in both key components of the registration process, **geometric transformation with interpolation** and **similarity metric** computation, by optimally mapping computational steps to heterogeneous hardware components on the [Versal VCK5000](https://japan.xilinx.com/content/dam/xilinx/publications/product-briefs/amd-xilinx-vck5000-product-brief.pdf).

## System architecture
***TODO upload architecture diagram***

## Code overview
- `3DIRG_application/`: complete ragistration framework
- `aie/`: AI Engines source code
- `common/`: constants and configuration generator
- `data_movers/`: PL kernels source code
- `hw/`: system integration and output bitstream
- `mutual_info/`: PL mutual information kernel source code
- `sw/`: host source code
- `default.cfg`: architecutre configuration parameters

## Artifact evaluation
### Case 1. Single registration step or only geometric transofmation
1. Choose a bitstream in `bitstreams/only_TX` or `bitstreams/reg_step` and copy it into `hw/`
2. Compile the host code: `make build_sw TASK=[TX|STEP]`
3. Pack the build into a single folder, ready for testing: `make pack [NAME=<name>]` (default name is `hw_build`)
4. Move into the generated folder under `build/` (i.e. `cd build/hw_build`)
5. Run: `./host_overlay.exe [depth] [x] [y] [ang_degrees] [num_runs]`

#### Plotting
To plot the... ***TODO write instructions***

### Case 2. Complete image registration application 
1. Choose a bitstream in `bitstreams/reg_step` and copy it into ??
2. ***TODO complete the instructions***

#### Plotting
To plot the... ***TODO write instructions***
