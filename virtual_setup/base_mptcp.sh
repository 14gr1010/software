#!/bin/bash

#wan=eth0
wan=wlan0
share=$PWD/../
img=ubuntu64-1-mptcp.img


# Make sure we are inside a screen
if [ $TERM != "screen" ]; then
    echo Don\'t we want to be inside a screen?
    exit 1
fi

./of_stop.sh

# Setup NAT
sudo ip_forward 1
sudo iptables -t nat -A POSTROUTING -o $wan -j MASQUERADE
sudo ip tuntap add tap1 mode tap
sudo ip address add dev tap1 10.0.2.101/24
sudo ip link set dev tap1 up
sudo ip route add to 10.0.2.1 dev tap1
sudo iptables -A FORWARD -i tap1 -j ACCEPT

sudo ip tuntap add of_tap1 mode tap
sudo ip link set dev of_tap1 up

tmux new-window -t 1 "kvm \
    -smp 1 \
    -drive file=$img,if=virtio \
    -m 512 \
    -netdev tap,ifname=tap1,script=no,downscript=no,id=lan0 \
    -device virtio-net-pci,netdev=lan0 \
    -netdev tap,ifname=of_tap1,script=no,downscript=no,id=lan1 \
    -device virtio-net-pci,mac=fe:fe:00:0a:01:01,netdev=lan1 \
    -fsdev local,security_model=passthrough,id=fsdev0,path=$share \
    -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare \
    -nographic"
