#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean; make
#declare -a ratio1trace=("deadlineCoflows_fanin_40_load_0.1" "deadlineCoflows_fanin_40_load_0.2" "deadlineCoflows_fanin_40_load_0.3")
declare -a ratio1trace=("fb_1hr_scaleRatio_10" "fb_1hr_scaleRatio_20" "fb_1hr_scaleRatio_30")

for trace in "${ratio1trace[@]}"
    do

    for top in 0 1 2 3
        do
        for nodeId in -1 0 128 256
            do
            for rt in 0
                do
                for trial in 0
                    do
                    fileName="top_"${top}"rt"${rt}"_nodeId_"${nodeId}"_trial_"${trial}"_"${trace}".temp"
                    echo $fileName
                    ./mainDynamic -topo ${top} -routing ${rt} -nodeId ${nodeId} -isddlflow 0 -trafficLevel 1 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
    done
    wait
    now=$(date +"%m_%d_%Y")
    linkFilename="final_nodes_"${trace}${now}".impact"
    echo $linkFilename
    for entry in ./*"node"*${trace}".temp"
        do
        cat ${entry} >> ${linkFilename}
        echo "" >> ${linkFilename}
    done
    rm *.temp
done