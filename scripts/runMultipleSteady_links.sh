#!/usr/bin/env bash
cd ../cmake-build-debug
cmake ..
make clean;make
declare -a arr=("perm_K16Ratio4server4096" "a2rack_K16Raio4RackSize32")
for trace in "${arr[@]}"
    do
    for top in 0 1 2
        do
        for linkNum in 0 1 3 5 10
            do
            for trial in 1 2
                do
                fileName="top_"$top"_linkNum_"$linkNum"_trial_"$trial".txt"
                ./main_MultipleSteadyLinkFailures -topo ${top} -routing 1 -linkNum "$linkNum" -switchNum 0 ../trafficTraces/$trace > ${fileName}
            done
        done
    done
done

for entry in ./top*.txt
    do
    cat ${entry} >> "final_links.txt"
    echo "" >> "final_links.txt"
done

