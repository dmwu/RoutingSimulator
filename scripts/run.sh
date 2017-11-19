#!/bin/bash
cd ../cmake-build-debug
cmake ..
make clean;make
for top in 0 1 2
    do
    for failLink in -1 0 1024 2048
        do
        for trial in 1 2
            do
            fileName="top_"$top"_link_"$failLink"_trial_"$trail".txt"
            echo $fileName
            ./main -o log  -topo "$top" -routing 1 -failLink "$failLink" ../trafficTraces/fb_5min_1 > "top_"$top"_link_"$failLink"_trial_"$trail".txt"
        done
    done
done

for entry in ./top*.txt
    do
    echo "$entry"
    tail -n 10 "$entry" >> "final.txt"
    echo "" >> "final.txt"
done
rm ./top*.txt


