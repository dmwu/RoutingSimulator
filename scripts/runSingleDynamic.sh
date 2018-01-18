#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean; make
declare -a ratio4trace=("deadlineCoflows_fanin_20")
declare -a ratio10trace=("perm_K16Ratio10server10240" "a2rack_K16Ratio10RackSize80")

for trace in "${ratio4trace[@]}"
    do
    for top in 0 1 2
        do
        fileName="top_"${top}"_link_reference"${trace}".temp"
        for linkId in -1 1024 2048
            do
            for rt in 0 1
                do
                for trial in 1 2 3
                    do
                    fileName="top_"${top}"rt"${rt}"_linkId_"${linkId}"_trial_"${trial}"_"${trace}".temp"
                    echo $fileName
                    ./mainDynamic -topo ${top} -routing ${rt} -linkId ${linkId} -isddlflow 1 -trafficLevel 0 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
        wait
    done
    now=$(date +"%m_%d_%Y")
    linkFilename="final_links_"${trace}${now}".impact"
    echo $linkFilename
    for entry in ./*"link"*${trace}".temp"
        do
        cat ${entry} >> ${linkFilename}
        echo "" >> ${linkFilename}
    done
    rm *.temp
done