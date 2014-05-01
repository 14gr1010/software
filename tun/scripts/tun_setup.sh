#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Remember device and ip"
    exit 1
fi

#dev=tun0
#ip=10.0.1.5
dev=$1
ip=$2
mtu=1350

sudo ip tuntap add dev $dev mode tun
sudo ip link set dev $dev mtu $mtu
sudo ip addr add $ip/24 dev $dev
sudo ip link set $dev up

#Delete
#ip tuntap del dev $dev mode tun

