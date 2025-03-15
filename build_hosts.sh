#!/bin/bash

# function to set a parameter in the configuration file
set_parameter() {
    if [ $# -ne 2 ]; then
        echo "Usage: $0 <parameter> <value>"
        exit 1
    fi

    PARAM=$1
    VALUE=$2

    echo "Setting $PARAM to $VALUE"

    sed -i "s/$PARAM := .*/$PARAM := $VALUE/" default.cfg

    if ! grep -q "$PARAM := $VALUE" default.cfg; then
        echo "Failed to set parameter ($PARAM := $VALUE)"
        echo "default.cfg might be broken"
        exit 1
    fi
}

echo "> SETTING PAPER CONFIGURATION"

set_parameter DIMENSION 512
set_parameter N_COUPLES_MAX 512
set_parameter HIST_PE 1
set_parameter ENTROPY_PE 1
set_parameter INT_PE 0
set_parameter PIXELS_PER_READ 32
set_parameter N_COUPLES 256

echo "--------------------"

# list of INT_PE to test
int_pe_list=(1 2 4 8 16 32)


echo "> ITERATING OVER THE FOLLOWING INT_PE VALUES: ${int_pe_list[*]}"

for int_pe in "${int_pe_list[@]}"
do
    echo "> BUILDING FOR INT_PE = $int_pe"
    make -C sw clean
    set_parameter INT_PE "$int_pe"
    make config TASK=TX
    make build_sw TASK=TX || exit 1
    folder_name=$(printf "onlyTX_%02dIPE" "$int_pe")
    xclbin_name=$(printf "onlyTX_%02dIPE.xclbin" "$int_pe")
    make pack NAME="$folder_name" XCLBIN="bitstreams/$xclbin_name" || exit 1
    echo "--------------------"
done


echo "> BUILDING REGISTRATION STEP"
set_parameter INT_PE 32
set_parameter HIST_PE 16
set_parameter ENTROPY_PE 4

make -C sw clean
make config TASK=STEP
make build_sw TASK=STEP || exit 1
folder_name=STEP_32IPE
xclbin_name=STEP_32IPE.xclbin
make pack NAME="$folder_name" XCLBIN="bitstreams/$xclbin_name" || exit 1
echo "--------------------"


echo "> BUILDING REGISTRATION Application"
folder_name="3DIRG_Application"
xclbin_name=STEP_32IPE.xclbin
make -C 3DIRG_application clean
make pack_app NAME="$folder_name" XCLBIN="bitstreams/$xclbin_name" || exit 1
echo "--------------------"
echo ""
echo "BUILD COMPLETE"
echo ""
echo "All builds have been placed under the folder build/"
ls -l build/

./finish_packing.sh
