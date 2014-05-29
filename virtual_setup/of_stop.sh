#!/bin/bash

# Reset
sudo killall -q qemu-system-x86_64
sudo killall -q ofdatapath
sudo iptables -F
sudo iptables -F -t nat
sudo ip_forward 0

sleep 1

for virt in veth of_tap tap ncif tun; do
    for dev in /sys/class/net/$virt*; do
        if [ "x$dev" == "x" ]; then
            break;
        fi
        # Check if removed with another veth:
        if [ -d $dev ]; then
            dev=${dev#/sys/class/net/}
            # echo veth dev: $dev
            sudo ip link set dev $dev down
            sudo ip link del $dev
        fi
    done
done

for i in $(seq 0 5); do
    if [ -d "/sys/class/net/br$i" ]; then
        sudo ip link set dev br$i down
        sudo ip link del br$i
    fi

    if [ -d "/sys/class/net/br$i" ]; then
        sudo ovs-vsctl del-br br$i
    fi

    if [ -f num$i.img ]; then
        rm num$i.img;
    fi
done
