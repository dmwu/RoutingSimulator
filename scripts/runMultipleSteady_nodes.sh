#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a ratio4trace=("newfb_10min_1")
declare -a ratio10trace=("perm_K16Ratio10server10240" "a2rack_K16Ratio10RackSize80")

for trace in "${ratio4trace[@]}"
    do
    for top in 0 1 2
        do
        fileName="top_"${top}"_switch_reference"${trace}".temp"
        ./mainSteady -topo ${top} -routing 0 -linkNum 0  -switchNum 0 -failurePos 0 -trafficLevel 0 -trial 1 ../trafficTraces/${trace} > ${fileName} &
        for switchNum in 1 2 3 4 5
            do
            for pos in 0 1 2
                do
                for trial in 1
                    do
                    fileName="top_"${top}"_switchNum_"${switchNum}"pos"${pos}"_trial_"${trial}"_"${trace}".temp"
                    ./mainSteady -topo ${top} -routing 0 -linkNum 0 -switchNum ${switchNum} -failurePos ${pos} -trafficLevel 0 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
        wait
    done
    now=$(date +"%m_%d_%Y")
    nodeFilename="final_nodes_"${trace}${now}".impact"
    for entry in ./*"switch"*${trace}".temp"
        do
        cat ${entry} >> ${nodeFilename}
        echo "" >> ${nodeFilename}
    done
    rm *.temp
done


