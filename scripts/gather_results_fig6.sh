# TODO copy csv from each folder and put them into the build/paper_fig/figure6/csv/ folder

# FOLDERS:
# onlyTX_01IPE
# onlyTX_02IPE
# onlyTX_04IPE
# onlyTX_08IPE
# onlyTX_16IPE
# onlyTX_32IPE

FOLDERS=(onlyTX_01IPE onlyTX_02IPE onlyTX_04IPE onlyTX_08IPE onlyTX_16IPE onlyTX_32IPE)

echo "Gathering results for figure 6..."

for folder in "${FOLDERS[@]}"
do
    if ! cp "$folder"/time_IPE*.csv paper_fig/figure6/csv/; then
        echo "Error: some csv files are missing in folder $folder"
        exit 1
    fi
done

echo "Done"
