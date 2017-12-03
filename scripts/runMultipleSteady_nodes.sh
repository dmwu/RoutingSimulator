#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a arr=("perm_K16Ratio4server4096" "a2rack_K16Raio4RackSize32")
for trace in "${arr[@]}"
    do
    for top in 0 1 2
        do
        for switchNum in 0 1 3 5 10
            do
            for trial in 1 2
                do
                fileName="top_"${top}"_switchNum_"${switchNum}"_trial_"${trial}".txt"
                ./main_MultipleSteadyLinkFailures -topo ${top} -routing 1 -linkNum 0 -switchNum ${switchNum} ../trafficTraces/${trace} > ${fileName}
            done
        done
    done
done

for entry in ./top*.txt
    do
    cat ${entry} >> "final_nodes.txt"
    echo "" >> "final_nodes.txt"
done