#!/bin/bash

./meas_stop.sh node2

mkdir -p results
ssh node2 "mkdir -p ~/results"

for i in $(seq 0 1 10)
do
    echo "Loss: $i Redundancy: XOR Adaptive"
    ssh node2 "iperf -s -y c >> ~/results/server_tcp_xora_$i.txt" &
    ssh node2 "iperf -su > /dev/null" &
    ssh node2 "~/master/code/tun/build/apps/pc_xora tun0 ~/results/server_tcp_xora_pkts_$i.txt 10.0.0.2 10.0.0.5 $i" &
    ../build/apps/pc_xora tun0 results/client_tcp_xora_pkts_$i.txt 10.0.0.5 10.0.0.2 0 &
    sleep 2
    iperf -c 10.0.1.2 -u -b 1m -l 100 -t 10 -r > /dev/null
    echo "print_drop_stat" > /dev/udp/127.0.0.1/54321
    ssh node2 "echo \"print_drop_stat\" > /dev/udp/127.0.0.1/54321"
    for n in $(seq 0 1 10)
    do
        iperf -c 10.0.1.2 -y c -t 50 >> results/client_tcp_xora_$i.txt
        echo "print_drop_stat" > /dev/udp/127.0.0.1/54321
        ssh node2 "echo \"print_drop_stat\" > /dev/udp/127.0.0.1/54321"
    done
    ./meas_stop.sh node2
    sleep 2
done
