#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a arr=("permutation_K16server4096" "all2OneRack_K16rackSize32")
for trace in "${arr[@]}"
    do
    for top in 0 1 2
        do
        for switchNum in 0 1 3 5 10
            do
            for trial in 1 2 3
                do
                fileName="top_"${top}"_switchNum_"${switchNum}"_trial_"${trial}".txt"
                echo $fileName
                ./main -topo "$top" -routing 1 -linkNum 0 -switchNum ${switchNum} ../trafficTraces/$trace > ${fileName}
            done
        done
    done
done

for entry in ./top*.txt
    do
    echo "$entry"
    tail -n 10 "$entry" >> "final_nodes.txt"
    echo "" >> "final_nodes.txt"
done