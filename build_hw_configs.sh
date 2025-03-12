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
    make clean
    set_parameter INT_PE "$int_pe"
    make config TASK=TX
    make build_hw TARGET=hw TASK=TX
    make build_sw TASK=TX
    folder_name=$(printf "onlyTX_%02dIPE" "$int_pe")
    make pack NAME="$folder_name"
    echo "--------------------"
done

echo "> BUILDING REGISTRATION STEP"
set_parameter INT_PE 32
set_parameter HIST_PE 16
set_parameter ENTROPY_PE 4


make clean
make config TASK=STEP
make build_hw TARGET=hw TASK=STEP
make build_sw TASK=STEP
folder_name=$(printf "STEP_%02dIPE" "$int_pe")
make pack NAME="$folder_name"   
echo "--------------------"

echo "> BUILDING FULL 3DIRG APP"
mkdir build/3DIRG_Application/
make pack_app NAME="3DIRG_Application"

echo "> BUILDING FIGURE_8 REQUIREMENTS"
mkdir build/figure8steps/realvolume_onlyTX_32IPE
mkdir build/figure8steps/3DIRG_app_build
cp -r build/onlyTX_32IPE/* build/figure8steps/realvolume_onlyTX_32IPE/
cp -r build/3DIRG_Application/* build/figure8steps/3DIRG_app_build/