#!/bin/bash

# IP-address of the local physical network interface
local_ip=10.0.0.5

# IP-address of the remote physical network interface
remote_ip=10.0.0.2

# IP-address of the coding interface on the server
test_ip=10.0.1.2

# IP-address of the remote physical network interface, used for signalling (may be identical to remote_ip)
remote=192.168.1.14

# Name of the physical network interface (both local and remote)
DEV=wlan0

#######################################x######################################
############################# DO NOT EDIT BELOW! #############################
##############################################################################

if [ $EUID -ne 0 ]; then
    echo "Run as root!"
    exit 1
fi

DELAY=0
pause=2

mkdir -p results
ssh $remote "mkdir -p /home/core/results"

for i in $(seq 1 10); do
    for ERROR_RATE in 0 0.5 1 2 3 4 5 6 8 10; do
        echo "Setting error rate=$ERROR_RATE"
        tc qdisc add dev $DEV root netem loss $ERROR_RATE || exit 1
        ssh $remote "tc qdisc add dev $DEV root netem loss $ERROR_RATE" || exit 2

        for REDUNDANCY in 5 3 0; do
            echo "i: $i Delay: $DELAY Loss: $ERROR_RATE Redundancy: xor $REDUNDANCY"
            ssh $remote "iperf -s -y c >> /home/core/results/server_tcp_xor$REDUNDANCY\_$ERROR_RATE\_$DELAY.txt" &
            ssh $remote "/home/core/master/code/tun/build/apps/singlepath/pc_xor tun0 $REDUNDANCY /home/core/results/server_tcp_xor$REDUNDANCY\_$ERROR_RATE\_$DELAY\_decode.txt $remote_ip $local_ip" &
            ../build/apps/singlepath/pc_xor tun0 $REDUNDANCY results/client_tcp_xor$REDUNDANCY\_$ERROR_RATE\_$DELAY\_decode.txt $local_ip $remote_ip &
            sleep $pause
            iperf -c $test_ip -y c -t 60 >> results/client_tcp_xor$REDUNDANCY\_$ERROR_RATE\_$DELAY.txt
            sleep $pause
            ssh $remote "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
            echo "print_decode_stat" > /dev/udp/127.0.0.1/54321
            sleep $pause
            ./stop.sh pc_xor $remote
            sleep $pause
        done

        for REDUNDANCY in 120 140 200 300; do
            echo "i: $i Delay: $DELAY Loss: $ERROR_RATE Redundancy: kodo $REDUNDANCY"
            ssh $remote "iperf -s -y c >> /home/core/results/server_tcp_kodo$REDUNDANCY\_$ERROR_RATE\_$DELAY.txt" &
            ssh $remote "/home/core/master/code/tun/build/apps/singlepath/pc_kodo tun0 $REDUNDANCY /home/core/results/server_tcp_kodo$REDUNDANCY\_$ERROR_RATE\_$DELAY\_decode.txt $remote_ip $local_ip" &
            ../build/apps/singlepath/pc_kodo tun0 $REDUNDANCY results/client_tcp_kodo$REDUNDANCY\_$ERROR_RATE\_$DELAY\_decode.txt $local_ip $remote_ip &
            sleep $pause
            iperf -c $test_ip -y c -t 60 >> results/client_tcp_kodo$REDUNDANCY\_$ERROR_RATE\_$DELAY.txt
            sleep $pause
            ssh $remote "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
            echo "print_decode_stat" > /dev/udp/127.0.0.1/54321
            sleep $pause
            ./stop.sh pc_kodo $remote
            sleep $pause
        done

        REDUNDANCY_KODO=140
        for REDUNDANCY_XOR in 0 5; do
            echo "i: $i Delay: $DELAY Loss: $ERROR_RATE Redundancy: kodo $REDUNDANCY_KODO XOR $REDUNDANCY_XOR"
            ssh $remote "iperf -s -y c >> /home/core/results/server_tcp_kodo$REDUNDANCY_KODO\_xor$REDUNDANCY_XOR\_$ERROR_RATE\_$DELAY.txt" &
            ssh $remote "/home/core/master/code/tun/build/apps/singlepath/pc_kodo_xor tun0 $REDUNDANCY_KODO $REDUNDANCY_XOR /home/core/results/server_tcp_kodo$REDUNDANCY_KODO\_xor$REDUNDANCY_XOR\_$ERROR_RATE\_$DELAY\_decode.txt $remote_ip $local_ip" &
            ../build/apps/singlepath/pc_kodo_xor tun0 $REDUNDANCY_KODO $REDUNDANCY_XOR results/client_tcp_kodo$REDUNDANCY_KODO\_xor$REDUNDANCY_XOR\_$ERROR_RATE\_$DELAY\_decode.txt $local_ip $remote_ip &
            sleep $pause
            iperf -c $test_ip -y c -t 60 >> results/client_tcp_kodo$REDUNDANCY_KODO\_xor$REDUNDANCY_XOR\_$ERROR_RATE\_$DELAY.txt
            sleep $pause
            ssh $remote "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
            echo "print_decode_stat" > /dev/udp/127.0.0.1/54321
            sleep $pause
            ./stop.sh pc_kodo_xor $remote
            sleep $pause
        done

        echo "Removing error rate=$ERROR_RATE"
        tc qdisc del dev $DEV root netem loss $ERROR_RATE || exit 3
        ssh $remote "tc qdisc del dev $DEV root netem loss $ERROR_RATE" || exit 4
    done
done
