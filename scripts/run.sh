#!/bin/bash
cd ../cmake-build-debug
cmake ..
make
for top in 0 1 2
    do
    for failLink in 0 1024 2048
        do
        for trail in 1 2 3
            do
            fileName = "top_"$top"_link_"$failLink"_trail_"$trail".txt"
            echo $fileName
            ./main -o log  -topo "$top" -routing 1 -failLink "$failLink" trafficTraces/fb_5min_1 > "top_"$top"_link_"$failLink"_trail_"$trail".txt"
        done
    done
done


