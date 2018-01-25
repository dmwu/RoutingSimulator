#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a ratio4trace=("deadlineCoflows_fanin_20")
declare -a ratio10trace=("perm_K16Ratio10server10240" "a2rack_K16Ratio10RackSize80")
declare -a ratio1trace=("fb1hr_scaleRatio_10" "fb1hr_scaleRatio_20" "fb1hr_scaleRatio_30")

for trace in "${ratio1trace[@]}"
    do
    for top in 0 1 2 3
        do
        fileName="top_"${top}"_switch_reference"${trace}".temp"
        ./mainSteady -topo ${top} -routing 1 -linkNum 0  -switchNum 0 -failurePos 0 -isddlflow 1 -trafficLevel 1 -trial 0 ../trafficTraces/${trace} > ${fileName} &
        for switchNum in 1 2 3 5 7
            do
            for pos in 1
                do
                for trial in 0
                    do
                    fileName="top_"${top}"_switchNum_"${switchNum}"pos"${pos}"_trial_"${trial}"_"${trace}".temp"
                    ./mainSteady -topo ${top} -routing 1 -linkNum 0 -switchNum ${switchNum} -failurePos ${pos} -isddlflow 1 -trafficLevel 1 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
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


