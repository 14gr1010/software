#!/bin/bash

./meas_stop.sh node2

mkdir -p results
ssh node2 "mkdir -p ~/results"

for i in $(seq 0 1 10)
do
    for x in 0 2 3 4 5 6
    do
        echo "Loss: $i Redundancy: XOR $x"
        ssh node2 "iperf -su -y c >> ~/results/server_udp_xor$x\_$i.txt" &
        ssh node2 "~/master/code/tun/build/apps/pc_xor tun0 ~/results/server_udp_xor$x\_pkts_$i.txt 10.0.0.2 10.0.0.5 $i $x" &
        ../build/apps/pc_xor tun0 results/client_udp_xor$x\_pkts_$i.txt 10.0.0.5 10.0.0.2 0 $x &
        sleep 2
        for n in $(seq 0 1 10)
        do
            iperf -c 10.0.1.2 -y c -t 50 -u -l 1322 -b 23m >> results/client_udp_xor$x\_$i.txt
            echo "print_drop_stat" > /dev/udp/127.0.0.1/54321
            ssh node2 "echo \"print_drop_stat\" > /dev/udp/127.0.0.1/54321"
        done
        ./meas_stop.sh node2
        sleep 2
    done
done
