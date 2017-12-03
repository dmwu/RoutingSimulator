#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a ratio4trace=("perm_K16Ratio4server4096" "a2rack_K16Raio4RackSize32")
declare -a ratio10trace=("perm_K16Ratio10server10240" "a2rack_K16Raio10RackSize80")

for trace in "${ratio4trace[@]}"
    do
    for top in 0 1 2
        do
        for linkNum in 0 1 3 5 10
            do
            for trial in 1 2 3
                do
                fileName="top_"${top}"_linkNum_"${linkNum}"_trial_"${trial}"_"${trace}".temp"
                ./mainSteady -topo ${top} -routing 1 -linkNum ${linkNum}  -switchNum 0 ../trafficTraces/${trace} > ${fileName} &
            done
        done
    done
    wait

    linkFilename="final_links_"${trace}".res"
    for entry in ./*"link"*${trace}".temp"
        do
        cat ${entry} >> ${linkFilename}
        echo "" >> ${linkFilename}
    done

    for top in 0 1 2
        do
        for switchNum in 0 1 3 5 10
            do
            for trial in 1 2
                do
                fileName="top_"${top}"_switchNum_"${switchNum}"_trial_"${trial}"_"${trace}".temp"
                ./mainSteady -topo ${top} -routing 1 -linkNum 0 -switchNum ${switchNum} ../trafficTraces/${trace} > ${fileName} &
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

done


