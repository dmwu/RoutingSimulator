#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a arr=("perm_K16Ratio4server4096" "perm_K16Ratio10server10240" "a2rack_K16Raio4RackSize32" "a2rack_K16Raio10RackSize80")
for trace in "${arr[@]}"
    do
    for top in 0 1 2
        do
        for linkNum in 0 1 3 5 10
            do
            for trial in 1 2
                do
                fileName="top_"${top}"_linkNum_"${linkNum}"_trial_"${trial}"_"${trace}".temp"
                ./mainSteady -topo ${top} -routing 1 -linkNum ${linkNum}  -switchNum 0 ../trafficTraces/${trace} > ${fileName}
            done
        done
    done

    resultFilename="final_links_"${trace}".res"
    for entry in ./*${trace}".temp"
        do
        cat ${entry} >> ${resultFilename}
        echo "" >> ${resultFilename}
    done
done

