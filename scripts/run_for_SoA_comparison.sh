#!/bin/bash

n_couples=256
tx=30
ty=30
ang=25
reps=50

./generate_dataset.sh 512 256

echo "> Running"
./host_overlay.exe $n_couples $tx $ty $ang $reps 
echo "--------------------"
