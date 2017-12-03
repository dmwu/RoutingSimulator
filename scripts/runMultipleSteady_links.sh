#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a arr=("permutation_K16server4096" "all2OneRack_K16rackSize32")
for trace in "${arr[@]}"
    do
    for top in 0 1 2
        do
        for failLinkNum in 0 1 3 5 10
            do
            for trial in 1 2 3
                do
                fileName="top_"$top"_linkNum_"$failLinkNum"_trial_"$trial".txt"
                echo $fileName
                ./main -o log  -topo "$top" -routing 1 -linkNum "$failLinkNum" -switchNum 0 ../trafficTraces/$trace > ${fileName}
            done
        done
    done
done

for entry in ./top*.txt
    do
    echo "$entry"
    tail -n 10 "$entry" >> "final_links.txt"
    echo "" >> "final_links.txt"
done

