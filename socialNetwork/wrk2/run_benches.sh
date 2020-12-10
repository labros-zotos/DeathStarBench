#!/bin/bash

connections=( 50 200 )
reqpersecond=( 10 100 250 500 1000 2000 )
cluster_ip=10.101.21.23

for c in "${connections[@]}"
do
    for r in "${reqpersecond[@]}"
    do
        ./wrk -D exp -t 2 -c $c -d 60s -L -s ./scripts/social-network/compose-post.lua http://$cluster_ip:8080/wrk2-api/post/compose -R $r > outs/c$c\_r$r.out
        echo "Done: Connections $c and $r req/sec. Output saved in outs folder"
    done
done
