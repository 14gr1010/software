#!/bin/bash

./meas_stop.sh --local-only

mkdir -p results

for i in $(seq 0 1 10)
do
    for x in 1
    do
        echo "Loss: $i Redundancy: XOR $x"
        iperf -s -y c >> results/local_server_tcp_xor$x\_$i.txt &
        ../build/apps/pc_local $i $x &
        sleep 2
        for n in $(seq 0 1 5)
        do
            iperf -c 10.0.1.105 -y c -t 10 >> results/local_client_tcp_xor$x\_$i.txt
            echo "print_drop_stat" > /dev/udp/127.0.0.1/55565
            echo "print_drop_stat" > /dev/udp/127.0.0.1/55566
        done
        ./meas_stop.sh --local-only
        sleep 2
    done
done

for i in $(seq 9 1 10)
do
    for x in 0 2 3 4 5 6
    do
        echo "Loss: $i Redundancy: XOR $x"
        iperf -s -y c >> results/local_server_tcp_xor$x\_$i.txt &
        ../build/apps/pc_local $i $x &
        sleep 2
        for n in $(seq 0 1 5)
        do
            iperf -c 10.0.1.105 -y c -t 10 >> results/local_client_tcp_xor$x\_$i.txt
            echo "print_drop_stat" > /dev/udp/127.0.0.1/55565
            echo "print_drop_stat" > /dev/udp/127.0.0.1/55566
        done
        ./meas_stop.sh --local-only
        sleep 2
    done
done
