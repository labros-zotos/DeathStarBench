#!/bin/bash

connections=( 10 50 200 300 500 )
reqpersecond=( 10 100 250 500 1000 2000 )
cluster_ip=10.100.189.154

for c in "${connections[@]}"
do
    for r in "${reqpersecond[@]}"
    do
        ./wrk -D exp -t 4 -c $c -d 60s -L -s ./scripts/media-microservices/compose-review.lua http://10.100.189.154:8080/wrk2-api/review/compose -R $r > outs/c$c\_r$r.out
        echo "Done: Connections $c and $r req/sec. Output saved in outs folder"
    done
done

