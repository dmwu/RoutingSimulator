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
        for linkNum in 1 3 5 10 15
            do
            for pos in 0 1 2
                do
                for trial in 1
                    do
                    fileName="top_"${top}"_linkNum_"${linkNum}"pos"${pos}"_trial_"${trial}"_"${trace}".temp"
                    ./mainSteady -topo ${top} -routing 0 -linkNum ${linkNum}  -switchNum 0 -failurePos ${pos} -trafficLevel 1 ../trafficTraces/${trace} > ${fileName} &
                done
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
    rm *.temp

#    for top in 0 2
#        do
#        for switchNum in 1 2 3
#            do
#            for pos in 0 1 2
#                do
#                for trial in 1
#                    do
#                    fileName="top_"${top}"_switchNum_"${switchNum}"pos"${pos}"_trial_"${trial}"_"${trace}".temp"
#                    ./mainSteady -topo ${top} -routing 0 -linkNum 0 -switchNum ${switchNum} -failurePos ${pos} -trafficLevel 1 ../trafficTraces/${trace} > ${fileName} &
#                done
#            done
#        done
#    done
#    wait
#
#    nodeFilename="final_nodes_"${trace}".res"
#    for entry in ./*"switch"*${trace}".temp"
#        do
#        cat ${entry} >> ${nodeFilename}
#        echo "" >> ${nodeFilename}
#    done
#    rm *.temp
done


