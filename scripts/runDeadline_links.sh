#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean; make
declare -a ratio1trace=("fb1hr_scaleRatio_10" "fb1hr_scaleRatio_20" "fb1hr_scaleRatio_30")
#declare -a ratio1trace=("newfb10min_scaleRatio_10" "newfb10min_scaleRatio_20" "newfb10min_scaleRatio_30")
#declare -a ratio1trace=("newfb10min_scaleRatio_20")
for trace in "${ratio1trace[@]}"
    do
    for top in 0 1 2 3
        do
        for linkId in -1 0 1024 2048
            do
            for rt in 0
                do
                for trial in 0
                    do
                    fileName="top_"${top}"rt"${rt}"_linkId_"${linkId}"_trial_"${trial}"_"${trace}".temp"
                    echo $fileName
                    ./mainDynamic -topo ${top} -routing ${rt} -linkId ${linkId} -isddlflow 1 -trafficLevel 1 -trial ${trial} ../trafficTraces/${trace} > ${fileName} &
                done
            done
        done
    done
    wait
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