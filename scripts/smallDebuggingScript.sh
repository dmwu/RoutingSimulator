#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a ratio4trace=("newfb_2min_1")
for trace in "${ratio4trace[@]}"
do
    for top in 0 1 2
        do
        for switchNum in 1 3
            do
            for pos in 0 1 2
                do
                for trial in 1
                    do
                    fileName="top_"${top}"_switchNum_"${switchNum}"pos"${pos}"_trial_"${trial}"_"${trace}".temp"
                    ./mainSteady -topo ${top} -routing 0 -linkNum 0 -switchNum ${switchNum} -failurePos ${pos} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
    done
    wait

    nodeFilename="final_nodes_"${trace}".res"
    for entry in ./*"switch"*${trace}".temp"
        do
        cat ${entry} >> ${nodeFilename}
        echo "" >> ${nodeFilename}
    done
    rm *.temp
done