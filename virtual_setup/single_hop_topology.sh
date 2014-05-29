#!/bin/bash

img=ubuntu64-1.img
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

# Setup NAT
sudo ip_forward 1
sudo iptables -t nat -A POSTROUTING -o $wan -j MASQUERADE
for i in $(seq 1 2); do
    sudo ip tuntap add tap$i mode tap
    sudo ip address add dev tap$i 10.0.2.10$i/24
    sudo ip link set dev tap$i up
    sudo ip route add to 10.0.2.$i dev tap$i
    sudo iptables -A FORWARD -i tap$i -j ACCEPT

    sudo ip tuntap add of_tap$i mode tap

    ### OVS setup
    sudo ovs-vsctl add-br br$i
    sudo ovs-vsctl set bridge br$i other-config:hwaddr=fe:fe:00:0a:FF:0$i

    ### Connet device to bridge
    sudo ovs-vsctl add-port br$i of_tap$i

    sudo ip link set dev of_tap$i up

    sudo ip link set dev br$i up

#     # Create image
    qemu-img create -f qcow2 -b $img num$i.img
done

# Create veth interface
sudo ip link add veth1 type veth peer name veth2
sudo ip link set veth1 up
sudo ip link set veth2 up
sudo ovs-vsctl add-port br1 veth1
sudo ovs-vsctl add-port br2 veth2


# sudo ip link set dev br0 up

echo Opening KVM

tmux new-window -d -t 1 "kvm \
    -smp 1 \
    -drive file=num1.img,if=virtio \
    -m 1024 \
    -nographic \
    -netdev tap,ifname=tap1,script=no,downscript=no,id=lan0 \
    -device virtio-net-pci,netdev=lan0 \
    -net nic,macaddr=fe:fe:00:0a:01:01 \
    -net tap,ifname=of_tap1,script=no \
    -fsdev local,security_model=passthrough,id=fsdev0,path=$share \
    -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare"

tmux new-window -d -t 2 "kvm \
    -smp 1 \
    -drive file=num2.img,if=virtio \
    -m 1024 \
    -nographic \
    -netdev tap,ifname=tap2,script=no,downscript=no,id=lan0 \
    -device virtio-net-pci,netdev=lan0 \
    -net nic,macaddr=fe:fe:00:0a:02:01 \
    -net tap,ifname=of_tap2,script=no \
    -fsdev local,security_model=passthrough,id=fsdev0,path=$share \
    -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=hostshare"

echo Setting switch to listen for controller on local interface

### OSV setup
sudo ovs-vsctl set-controller br1 tcp:0.0.0.0:6633
sudo ovs-vsctl set-controller br2 tcp:0.0.0.0:6633

echo "Waiting 20s for nodes to start..."
sleep 20
echo "Setting up hostshare"

## mount hostshare
for i in $(seq 1 2); do
    ssh node@n$i "sudo mount hostshare"
    ssh node@n$i "~/share/tun/scripts/tun_setup.sh ncif 10.1.1.$i"
done

tmux new-window -dk -t 9 -c $PWD/../virtual_runners/

tmux new-window -k -t 8 "ssh node@n2 'iperf -s'"
