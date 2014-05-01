#!/bin/bash

killall pc_kodo
killall pc_local
killall pc_multisetup
killall pc_no
killall pc_xor
killall pc_xora

for k in $(seq 0 1 3)
do
    killall iperf
done

if [ "$1" == "--local-only" ]; then
    exit
fi

remote=$1

ssh $remote "killall pc_kodo"
ssh $remote "killall pc_local"
ssh $remote "killall pc_multisetup"
ssh $remote "killall pc_no"
ssh $remote "killall pc_xor"
ssh $remote "killall pc_xora"

for k in $(seq 0 1 3)
do
    ssh $remote "killall iperf"
done

