#!/bin/bash

img=ubuntu64-1-mptcp.img
share=$PWD/../
nwan=eth0
wan=wlan0
addr=10.0.2.100/24
sub=10.0.2.0/24


# Make sure we are inside a screen
if [ $TERM != "screen" ]; then
    echo Don\'t we want to be inside a screen?
    exit 1
fi

./of_stop.sh


# Setup:
#              br2
#          +----X---+
# mp coder |br1     |br4    mp decoder
#  n1  0---X--------X-----0 n2
#          |        |
#          +----X---+
#              br3
#
# n1 has virtual devices of_tap11 of_tap12 of_tap13
# n2 has virtual devices of_tap21 of_tap22 of_tap23
# of_tap11 has hwaddr is fe:fe:00:0a:01:01
# of_tap12 has hwaddr is fe:fe:00:0a:01:02
# of_tap13 has hwaddr is fe:fe:00:0a:01:03
# of_tap21 has hwaddr is fe:fe:00:0a:02:01
# of_tap22 has hwaddr is fe:fe:00:0a:02:02
# of_tap23 has hwaddr is fe:fe:00:0a:02:03
# oftap11 and oftap 21 is connected to br1 and br4 respectively
#  the remainders are not connected (communicates through the connected devs)
#
# br'x' is connected to br'y' with virtual ethernet ifs veth'xy'-veth'yx'

# Setup NAT
# sudo ip_forward 1
# sudo iptables -t nat -A POSTROUTING -o $wan -j MASQUERADE
for i in $(seq 1 2); do
    sudo ip tuntap add tap$i mode tap
    sudo ip address add dev tap$i 10.0.2.10$i/24
    sudo ip link set dev tap$i up
    sudo ip route add to 10.0.2.$i dev tap$i
    sudo iptables -A FORWARD -i tap$i -j ACCEPT

    for j in $(seq 1 3); do
    # Create tap for the virtual network
        sudo ip tuntap add of_tap$i$j mode tap

        # Activate devices
        sudo ip link set dev of_tap$i$j up
    done

#     # Create image
    qemu-img create -f qcow2 -b $img num$i.img
done

# SETUP OVSWITCH
for i in $(seq 1 4); do
    sudo ovs-vsctl add-br br$i
    sudo ovs-vsctl set bridge br$i other-config:hwaddr=fe:fe:00:0a:FF:0$i
    # Create bridge

    sudo ip link set dev br$i up
done

# Setup virtual machine to br1 and br2 accordingly
sudo ovs-vsctl add-port br1 of_tap11
sudo ovs-vsctl add-port br4 of_tap21

## Setup virtual ethernet links:
sudo ip link add veth12 type veth peer name veth21
sudo ip link add veth13 type veth peer name veth31
sudo ip link add veth14 type veth peer name veth41
sudo ip link add veth24 type veth peer name veth42
sudo ip link add veth34 type veth peer name veth43

## Setup veth's to switches:
sudo ovs-vsctl add-port br1 veth12
sudo ovs-vsctl add-port br1 veth13
sudo ovs-vsctl add-port br1 veth14
sudo ovs-vsctl add-port br2 veth21
sudo ovs-vsctl add-port br2 veth24
sudo ovs-vsctl add-port br3 veth31
sudo ovs-vsctl add-port br3 veth34
sudo ovs-vsctl add-port br4 veth41
sudo ovs-vsctl add-port br4 veth42
sudo ovs-vsctl add-port br4 veth43

echo Opening KVM

for i in $(seq 1 2); do
    tmux new-window -t $i "kvm \
    -smp 1 \
    -drive file=num$i.img,if=virtio \
    -m 1024 \
    -nographic \
    -netdev tap,ifname=tap$i,script=no,downscript=no,id=lan0 \
    -device virtio-net-pci,netdev=lan0 \
    -net nic,macaddr=fe:fe:00:0a:0$i:01 \
    -net tap,ifname=of_tap$i\1,script=no \
    -net nic,macaddr=fe:fe:00:0a:0$i:02 \
    -net tap,ifname=of_tap$i\2,script=no \
    -net nic,macaddr=fe:fe:00:0a:0$i:03 \
    -net tap,ifname=of_tap$i\3,script=no \
    -fsdev local,security_model=passthrough,id=fsdev0,path=$share \
    -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare"
done

echo Setting switch to listen for controller on local interface

### Set openflow controller address
for i in $(seq 1 4); do
    sudo ovs-vsctl set-controller br$i tcp:0.0.0.0:6633
done

## Start openflow controller - launch openflow.discovery for link discovery
echo Setting up controller

tmux new-window -dk -t 5 "$PWD/../pox/pox.py log.level --WARNING openflow.discovery mphardswitch"

echo "Waiting 20s for nodes to start..."
sleep 1

echo "Setting up links in the network"

# Set all veth links active
for dev in /sys/class/net/veth*; do
    if [ "x$dev" == "x" ]; then
        echo "COULD NOT FIND VETH DEVICES!"
        break; # No devices
    fi
    dev=${dev#/sys/class/net/}
    sudo ip link set dev $dev up
done

echo "Finished setting up links in the network"

sleep 19


echo "Setting up coding and arp"
# mount hostshare
for i in $(seq 1 2); do
    ssh node@n$i "sudo mount hostshare"
    ssh node@n$i "~/share/tun/scripts/tun_setup.sh ncif 10.1.1.$i"

    j=1
    if [ $i == 1 ]; then
        j=2
    fi

    ssh node@n$i "sudo arp -s 10.0.1.$j fe:fe:00:0a:0$j:01"
    ssh node@n$i "sudo arp -s 10.0.12.$j fe:fe:00:0a:0$j:02"
    ssh node@n$i "sudo arp -s 10.0.13.$j fe:fe:00:0a:0$j:03"

done

tmux new-window -k -t 9 -c $PWD/../virtual_runners

tmux new-window -dk -t 8 "ssh node@n2 'iperf -s'"
