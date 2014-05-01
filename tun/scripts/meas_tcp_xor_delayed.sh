#!/bin/bash

if [ $EUID -ne 0 ]; then
    echo "Run as root!"
    exit 1
fi

./meas_stop.sh $remote

remote=192.168.1.14

mkdir -p results
ssh $remote "mkdir -p /home/core/results"

for n in $(seq 0 1 10)
do
    for d in 1 100 300 500
    do
        tc qdisc add dev wlan0 root netem delay ${d}ms
        ssh $remote "tc qdisc add dev wlan0 root netem delay ${d}ms"
        for i in $(seq 0 1 10)
        do
            for x in 0 2 3 4 5
            do
                echo "Loss: $i Redundancy: XOR $x Delay: ${d}ms N: $n"
                ssh $remote "iperf -s -y c >> /home/core/results/server_tcp_xor$x\_$i\_$d\_long.txt" &
                ssh $remote "/home/core/master/code/tun/build/apps/pc_xor tun0 /home/core/results/server_tcp_xor$x\_pkts_$i\_$d\_long.txt 10.0.0.2 10.0.0.5 $i $x" &
                ../build/apps/pc_xor tun0 results/client_tcp_xor$x\_pkts_$i\_$d\_long.txt 10.0.0.5 10.0.0.2 0 $x &
                sleep 2
                iperf -c 10.0.1.2 -y c -t 500 >> results/client_tcp_xor$x\_$i\_$d\_long.txt
                echo "print_drop_stat" > /dev/udp/127.0.0.1/54321
                ssh $remote "echo \"print_drop_stat\" > /dev/udp/127.0.0.1/54321"
                sleep 2
                ./meas_stop.sh $remote
                sleep 2
            done
        done
        tc qdisc del dev wlan0 root netem delay ${d}ms
        ssh $remote "tc qdisc del dev wlan0 root netem delay ${d}ms"
    done
done
