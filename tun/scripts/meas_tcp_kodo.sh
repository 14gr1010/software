#!/bin/bash

if [ $EUID -ne 0 ]; then
    echo "Run as root!"
    exit 1
fi

local_ip=10.0.0.3
remote_ip=10.0.0.1
test_ip=10.0.1.1
remote=10.42.0.20
pause=2

./meas_stop.sh $remote

mkdir -p results
ssh $remote "mkdir -p /home/core/results"

for n in $(seq 1 10)
do
    for d in 1 10 100
    do
        echo "Adding delay"
        tc qdisc add dev wlan0 root netem delay ${d}ms || exit 1
        ssh $remote "tc qdisc add dev wlan0 root netem delay ${d}ms" || exit 1
        for i in $(seq 0 1 10)
        do
            for r in 120 140 #200 300
            do
                for x in 0 2 6
                do
                    echo "N: $n Delay: $d Loss: $i Redundancy: kodo $r XOR $x"
                    ssh $remote "iperf -s -y c >> /home/core/results/server_tcp_kodo$r\_xor$x\_$i\_$d\_medium.txt" &
                    ssh $remote "/home/core/master/code/tun/build/apps/pc_kodo tun0 /home/core/results/server_tcp_kodo$r\_xor$x\_pkts_$i\_$d\_medium.txt $remote_ip $local_ip $i $r $x" &
                    ../build/apps/pc_kodo tun0 results/client_tcp_kodo$r\_xor$x\_pkts_$i\_$d\_medium.txt $local_ip $remote_ip 0 $r $x &
                    sleep $pause
                    iperf -c $test_ip -y c -t 100 >> results/client_tcp_kodo$r\_xor$x\_$i\_$d\_medium.txt
                    echo "print_drop_stat" > /dev/udp/127.0.0.1/54321
                    ssh $remote "echo \"print_drop_stat\" > /dev/udp/127.0.0.1/54321"
                    sleep $pause
                    ./meas_stop.sh $remote
                    sleep $pause
                done
            done
        done
        echo "Removing delay"
        tc qdisc del dev wlan0 root netem delay ${d}ms || exit 1
        ssh $remote "tc qdisc del dev wlan0 root netem delay ${d}ms" || exit 1
    done
done

