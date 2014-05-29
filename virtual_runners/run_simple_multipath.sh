#!/bin/bash
#enc: utf8

# General parameters
LOGDIR=logs/
SERVER="node@n2"
CLIENT="node@n1"
CLIENT_IP1=10.0.1.1
CLIENT_IP2=10.0.12.1
CLIENT_IP3=10.0.13.1
SERVER_IP1=10.0.1.2
SERVER_IP2=10.0.12.2
SERVER_IP3=10.0.13.2
CODED_SERVER_IP=10.1.1.2

# Virtual ethernets are denoted vethf$i and vetht$i, from 1 to 5

MAXRATE=20mbit

# Iperf duration
DURATION=60

# Coder directory on the virtual machines
CODERDIR=/home/node/share/tun/build/apps/singlepath/
MPCODERDIR=/home/node/share/tun/build/apps/multipath/

# Coder logdir
CLOG=/home/node/share/logs

KODO_REDUNDANCY="120 140"

P1D=0; P2D=0; P3D=0

# Coders in use
# CODERS="pc_xor pc_kodo pc_kodo_xor"


function poll_recovery_stats {
    if [ $# != 0 ]; then
        echo "poll_recovery_stats argument mismatch"
        exit 1
    fi
    ssh $SERVER "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
    ssh $CLIENT "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"

    sleep 5
}

function run_iperf {
    if [ $# != 2 ]; then
        echo "run_iperf argument mismatch. Arguments needed: \"server_ip iperf_logfile\""
        exit 1
    fi
    ssh $CLIENT "iperf -c $1 -t $DURATION -y C" >> $2
}

function run_uncoded {
    if [ $# != 2 ]; then
        echo "run_uncoded argument mismatch. Arguments needed: \"server_ip iperf_logfile\""
        exit 1
    fi

    # tmux new-window -dk -t 6 "ssh $SERVER \"iperf -s\""

    run_iperf $1 $2
}

function run_kodo {
    if [ $# != 4 ]; then
        echo "run_kodo argument mismatch. Arguments needed: \"coding_server_ip iperf_logfile coding_logfile redundancy\""
        exit 1
    fi

    if [ "$4" -lt "100" ]; then
        echo kodo redundancy below 100. Not allowed.
        exit 1
    fi
    ## Setup
    CODER=pc_kodo

    # echo running kodo
    tmux new-window -dk -t 6 "ssh $CLIENT \"$CODERDIR$CODER ncif $4 /dev/null $CLIENT_IP1 $SERVER_IP1\""
    tmux new-window -dk -t 7 "ssh $SERVER \"$CODERDIR$CODER ncif $4 $3 $SERVER_IP1 $CLIENT_IP1\""

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall -q $CODER"
    ssh $SERVER "killall -q $CODER"
    sleep 2
}

function run_kodo_multipath {
# Args
# 1: coding_server_ip
# 2: iperf_logfile
# 3: coding_logfile
# 4: total redundancy
# 5: Path 1 data distribution
# 6: Path 2 data distribution
# 7: Path 3 data distribution

    if [ $# != 7 ]; then
        echo "run_kodo_multipath argument mismatch. See function definition for arguments needed."
        exit 1
    fi

    if [ $4 -lt 100 ]; then
        echo kodo redundancy below 100. Not allowed.
        exit 1
    fi

    if [ `expr $5 + $6 + $7` != 100 ]; then
        echo "Data distribution for multipaths not equal to 100\!"
        exit 1
    fi

    ## Setup
    COD=pc_kodo_dist
    CODER=$MPCODERDIR$COD

    tmux new-window -dk -t 6 \
        "ssh $CLIENT \"$CODER ncif $4 /dev/null $CLIENT_IP2 $SERVER_IP2 11111 11111 $5\
                                                $CLIENT_IP1 $SERVER_IP1 11112 11112 $6\
                                                $CLIENT_IP3 $SERVER_IP3 11113 11113 $7\""

    tmux new-window -dk -t 7 \
        "ssh $SERVER \"$CODER ncif $4 $3 $SERVER_IP2 $CLIENT_IP2 11111 11111 $5\
                                         $SERVER_IP1 $CLIENT_IP1 11112 11112 $6\
                                         $SERVER_IP3 $CLIENT_IP3 11113 11113 $7\""

    run_iperf $1 $2
    poll_recovery_stats

    sleep 2

    ssh $CLIENT "killall -q $COD"
    ssh $SERVER "killall -q $COD"
}

function veth_clean_tc {
    # Clean all veth devices for tc configuration
    for dev in /sys/class/net/veth*; do
        if [ "x$dev" == "x" ]; then
            break;
        fi
        dev=${dev#/sys/class/net/}
        sudo tc qdisc del dev $dev root
    done
}

function set_topology {
    if [ $# != 1 ]; then
        echo "argument mismatch in function set_topology"
        exit 1
    fi
    if [ $1 != 1 ] && [ $1 != 2 ] && [ $1 != 3 ] && [ $1 != 0 ]; then
        echo "Unknown argument to function set_topology: $1. Expected 1, 2 or 3"
        exit 1
    fi

    # Cleaning up:
    veth_clean_tc

    sudo tc qdisc add dev veth14 root netem \
        rate $MAXRATE delay 5ms 1ms distribution normal loss $ERROR_RATE

    sudo tc qdisc add dev veth41 root netem \
        rate $MAXRATE delay 5ms 1ms distribution normal loss $ERROR_RATE

    # Topology 0 setuo, side loss but same delay on all paths
    if [ $1 == 0 ]; then
         # Delay on first path, br2: 10ms, both directions
        sudo tc qdisc add dev veth24 root netem delay 5ms 1ms distribution normal rate $MAXRATE
        sudo tc qdisc add dev veth42 root netem delay 5ms 1ms distribution normal rate $MAXRATE

        # Delay on third path, br3, 20ms, both directions
        sudo tc qdisc add dev veth34 root netem delay 5ms 1ms distribution normal rate $MAXRATE
        sudo tc qdisc add dev veth43 root netem delay 5ms 1ms distribution normal rate $MAXRATE
    fi

    # Topology 1 setup, no side loss, just delay
    if [ $1 == 1 ]; then

        # Delay on first path, br2: 10ms, both directions
        sudo tc qdisc add dev veth24 root netem delay 10ms 2ms distribution normal rate $MAXRATE
        sudo tc qdisc add dev veth42 root netem delay 10ms 2ms distribution normal rate $MAXRATE

        # Delay on third path, br3, 20ms, both directions
        sudo tc qdisc add dev veth34 root netem delay 20ms 4ms distribution normal rate $MAXRATE
        sudo tc qdisc add dev veth43 root netem delay 20ms 4ms distribution normal rate $MAXRATE

        # Path 2 (br1-br4) will be sweeped over
    fi

    # # Topology 2 setup, small side loss losses
    if [ $1 == 2 ]; then

        # Delay and loss on first path - higher loss than third path (due to lower delay, only fair)
        sudo tc qdisc add dev veth24 root netem \
            delay 10ms 2ms distribution normal rate $MAXRATE loss 2
        sudo tc qdisc add dev veth42 root netem \
            delay 10ms 2ms distribution normal rate $MAXRATE loss 2

        # Delay on third path, br3, 20ms, both directions - bit of loss
        sudo tc qdisc add dev veth34 root netem \
            delay 20ms 4ms distribution normal rate $MAXRATE loss 1
        sudo tc qdisc add dev veth43 root netem \
            delay 20ms 4ms distribution normal rate $MAXRATE loss 1
    fi

    # Topology 3 setup, small side loss losses
    if [ $1 == 3 ]; then

        # Delay and loss on first path - higher loss than third path (due to lower delay, only fair)
        sudo tc qdisc add dev veth24 root netem \
            delay 10ms 2ms distribution normal rate $MAXRATE loss 4
        sudo tc qdisc add dev veth42 root netem \
            delay 10ms 2ms distribution normal rate $MAXRATE loss 4

        # Delay on third path, br3, 20ms, both directions - a little more loss than before
        sudo tc qdisc add dev veth34 root netem \
            delay 20ms 4ms distribution normal rate $MAXRATE loss 2
        sudo tc qdisc add dev veth43 root netem \
            delay 20ms 4ms distribution normal rate $MAXRATE loss 2
    fi
}


if [ $TERM != "screen" ]; then
    echo Don\'t we want to be inside a screen?
    exit 1
fi


veth_clean_tc

# Clean up and set maxrate on of_taps
for i in $(seq 1 2); do
    sudo tc qdisc del dev of_tap$i\1 root
    sudo tc qdisc add dev of_tap$i\1 root netem rate $MAXRATE
done

# First jumps are constant, just rate limited, no loss, no delay

# Path 1.1 config:
sudo tc qdisc add dev veth12 root netem rate $MAXRATE
sudo tc qdisc add dev veth21 root netem rate $MAXRATE

# Path 3.1 config
sudo tc qdisc add dev veth31 root netem rate $MAXRATE
sudo tc qdisc add dev veth13 root netem rate $MAXRATE



for CYCLE in $(seq 1 10); do

    for ERROR_RATE in 0 0.5 1 2 3 4 5 6 8 10
    do
        for i in $(seq 1 2); do
            sudo tc qdisc del dev of_tap$i\1 root
            sudo tc qdisc add dev of_tap$i\1 root netem rate $MAXRATE
        done

        echo Setting error rate to $ERROR_RATE percent loss


        sudo tc qdisc del dev veth14 root
        sudo tc qdisc del dev veth41 root

        sudo tc qdisc add dev veth14 root netem \
            rate $MAXRATE delay 5ms 1ms distribution normal loss $ERROR_RATE

        sudo tc qdisc add dev veth41 root netem \
            rate $MAXRATE delay 5ms 1ms distribution normal loss $ERROR_RATE


        echo Running uncoded iperf - $DURATION seconds
        IPERF_LOGFILE=iperf_multipath_uncoded_error_$ERROR_RATE.dat
        run_uncoded $SERVER_IP1 $LOGDIR$IPERF_LOGFILE
        sleep 1

        for RED in $KODO_REDUNDANCY; do
            echo Running kodo iperf - $DURATION seconds
            LOG=multipath_kodo_singlepath_topology_$t\_error_$ERROR_RATE\_redundancy_$RED.dat
            IPERF_LOGFILE=iperf_$LOG
            CODING_LOGFILE=recovery_$LOG
            run_kodo $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
            sleep 1
        done

        for t in 0 1 3; do  #$(seq 1 3); do

            echo "Running topology $t"
            set_topology $t

            for RED in $KODO_REDUNDANCY; do

                for i in $(seq 1 2); do
                    sudo tc qdisc del dev of_tap$i\1 root
                    MRATE=${MAXRATE%mbit}
                    RATE=$(expr $MRATE + $MRATE + $MRATE)
                    echo "Setting of_tap$i\1 rate to $RATE\mbit"
                    sudo tc qdisc add dev of_tap$i\1 root netem rate $RATE\mbit
                done

                ###### Dist 1: 33/RED, 100-2*(33/RED), 33/RED
                if [ $RED == 120 ]; then
                    P1D=28; P2D=44; P3D=28
                fi

                if [ $RED == 140 ]; then
                    P1D=24; P2D=52; P3D=24
                fi

                if [ $RED == 200 ]; then
                    P1D=17; P2D=66; P3D=17
                fi

                LOG=multipath_kodo_distributed_$P1D\_$P2D\_$P3D\_topology_$t\_error_$ERROR_RATE\_redundancy_$RED.dat

                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG

                echo "Running multipath kodo distribution 1, redundancy $RED, error $ERROR_RATE, top. $t. cycle $CYCLE"

                run_kodo_multipath $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE \
                    $RED $P1D $P2D $P3D

                sleep 1

                ######## Dist 2:


                P1D=33; P2D=34; P3D=33

                LOG=multipath_kodo_distributed_$P1D\_$P2D\_$P3D\_topology_$t\_error_$ERROR_RATE\_redundancy_$RED.dat

                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG

                echo "Running multipath kodo distribution 2, redundancy $RED, error $ERROR_RATE, top. $t. cycle $CYCLE"

                run_kodo_multipath $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE \
                    $RED $P1D $P2D $P3D

                sleep 1



                ####### Dist 3:

                P1D=40; P2D=20; P3D=40

                LOG=multipath_kodo_distributed_$P1D\_$P2D\_$P3D\_topology_$t\_error_$ERROR_RATE\_redundancy_$RED.dat

                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG

                echo "Running multipath kodo distribution 3, redundancy $RED, error $ERROR_RATE, top. $t. cycle $CYCLE"

                run_kodo_multipath $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE \
                    $RED $P1D $P2D $P3D

                sleep 1


            done
        done
    done
done

veth_clean_tc



# Clean up and set maxrate on of_taps
for i in $(seq 1 4); do
    sudo tc qdisc del dev of_tap$i root
    sudo tc qdisc add dev of_tap$i root netem rate $MAXRATE
done
