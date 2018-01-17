#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a ratio4trace=("newfb_10min_1")
declare -a ratio10trace=("perm_K16Ratio10server10240" "a2rack_K16Ratio10RackSize80")
#-topo 0 -routing 1 -linkId 4097 -trafficLevel 0 -trial 0 trafficTraces/singleFlowDebugging
for trace in "${ratio4trace[@]}"
    do
    for top in 0 1 2
        do
        fileName="top_"${top}"_link_reference"${trace}".temp"
        for linkId in 0 4096 5120
            do
            for rt in 0 1
                do
                for trial in 1 2 3 4 5
                    do
                    fileName="top_"${top}"rt"${rt}"_linkId_"${linkId}"_trial_"${trial}"_"${trace}".temp"
                    ./mainDynamic -topo ${top} -routing ${rt} -linkId ${linkId} -trafficLevel 1 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
        wait
    done
    now=$(date +"%m_%d_%Y")
    linkFilename="final_links_"${trace}${now}".impact"
    for entry in ./*"link"*${trace}".temp"
        do
        cat ${entry} >> ${linkFilename}
        echo "" >> ${linkFilename}
    done
    rm *.temp
done