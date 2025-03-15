#!/bin/bash

echo "Gathering results for figure 7..."

cp STEP_32IPE/time_IPE32_D512_N246.csv paper_fig/figure7/csv/TRILLI_32IPE_STEP.csv || exit 1
cp onlyTX_32IPE/time_IPE32_D512_N246.csv paper_fig/figure7/csv/TRILLI_32IPE_TX.csv || exit 1
cp 3DIRG_Application/TRILLI_pow.csv paper_fig/figure7/csv/

echo "Done"
