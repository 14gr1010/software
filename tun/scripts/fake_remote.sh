#!/bin/bash
unload=$1
iface_one=tun0
iface_two=tun1
local_one=10.0.1.5
local_two=10.0.1.6
remote_one=10.0.1.105
remote_two=10.0.1.106
mac_one=$(cat /sys/class/net/$iface_one/address)
mac_two=$(cat /sys/class/net/$iface_two/address)

# iptables rules
ipt_src_one="POSTROUTING -s $local_one/32 -d $remote_two/32 -o $iface_one -j SNAT --to-source $remote_one"
ipt_src_two="POSTROUTING -s $local_two/32 -d $remote_one/32 -o $iface_two -j SNAT --to-source $remote_two"
ipt_dst_one="PREROUTING -s $remote_two/32 -d $remote_one/32 -i $iface_one -j DNAT --to-destination $local_one"
ipt_dst_two="PREROUTING -s $remote_one/32 -d $remote_two/32 -i $iface_two -j DNAT --to-destination $local_two"

# routes
route_one="$remote_two dev $iface_one"
route_two="$remote_one dev $iface_two"

# neighbors
neigh_one="$remote_one dev $iface_two lladdr $mac_one"
neigh_two="$remote_two dev $iface_one lladdr $mac_two"


# remove old iptables rules
ipt=$(sudo iptables-save)
if echo $ipt | grep -q "$ipt_src_one"; then
    echo "delete iptables $ipt_src_one"
    sudo iptables -t nat -D $ipt_src_one
fi

if echo $ipt | grep -q "$ipt_src_two"; then
    echo "delete iptables $ipt_src_two"
    sudo iptables -t nat -D $ipt_src_two
fi

if echo $ipt | grep -q "$ipt_dst_one"; then
    echo "delete iptables $ipt_dst_one"
    sudo iptables -t nat -D $ipt_dst_one
fi

if echo $ipt | grep -q "$ipt_dst_two"; then
    echo "delete iptables $ipt_dst_two"
    sudo iptables -t nat -D $ipt_dst_two
fi

# remove old routes
routes=$(sudo ip route)
if echo $routes | grep -q "$route_one"; then
    echo "delete route $route_one"
    sudo ip route del $route_one
fi

if echo $routes | grep -q "$route_two"; then
    echo "delete route $route_two"
    sudo ip route del $route_two
fi

# remove old neighbors
#neighs=$(sudo ip neigh)
#if echo $neighs | grep -q "$neigh_one"; then
#    echo "delete neigh $neigh_one"
#    sudo ip neigh del $neigh_one
#fi
#
#neighs=$(sudo ip neigh)
#if echo $neighs | grep -q "$neigh_two"; then
#    echo "delete neigh $neigh_two"
#    sudo ip neigh del $neigh_two
#fi 
#
#neighs=$(sudo ip neigh)
#if echo $neighs | grep -q "$remote_two dev $iface_one"; then
#    echo "flush neighbor $remote_two dev $iface_one"
#    sudo ip link set dev $iface_one arp off
#    sudo ip link set dev $iface_one arp on
#fi
#
#neighs=$(sudo ip neigh)
#if echo $neighs | grep -q "$remote_one dev $iface_two"; then
#    echo "flush neighbor $remote_one dev $iface_two"
#    sudo ip link set dev $iface_two arp off
#    sudo ip link set dev $iface_two arp on
#fi

if [ "$unload" != "" ]; then
    exit 0
fi

# set routes
echo "add route $route_one"
sudo ip route add $route_one || exit 1
echo "add route $route_two"
sudo ip route add $route_two || exit 1

# add neighbor entries
#echo "add neigh $neigh_one"
#sudo ip neigh add $neigh_one || exit 1
#echo "add neigh $neigh_two"
#sudo ip neigh add $neigh_two || exit 1

# fake source addresses of outgoing packets
echo "add iptables $ipt_src_one"
sudo iptables -t nat -A $ipt_src_one || exit 1
echo "add iptables $ipt_src_two"
sudo iptables -t nat -A $ipt_src_two || exit 1

# replace incoming destination address to match local address
echo "add iptables $ipt_dst_one"
sudo iptables -t nat -A $ipt_dst_one || exit 1
echo "add iptables $ipt_dst_two"
sudo iptables -t nat -A $ipt_dst_two || exit 1

echo "done"
