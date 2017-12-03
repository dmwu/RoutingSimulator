#!/bin/bash
cd ../cmake-build-debug
cmake ..
make clean;make
for top in 0 1 2
    do
    for failLink in -1 0 1024 2048
        do
        for trial in 1
            do
            fileName="top_"$top"_link_"$failLink"_trial_"$trial".txt"
            echo $fileName
            ./main -o log  -topo "$top" -routing 1 -failLink "$failLink" ../trafficTraces/fb_10min_1 > $fileName
        done
    done
done

for entry in ./top*.txt
    do
    echo "$entry"
    tail -n 10 "$entry" >> "final.txt"
    echo "" >> "final.txt"
done


