#!/bin/bash
#enc: utf8

## This script runs iperf to measure bandwidth between two machines.
## Two iperf instances is run simultaneously to investigate the fairness of the underlying coding (if any).

LOGDIR=../logs/
LOGDIR_FAIRNESS="$LOGDIR/fairness"
LOGNORMAL=$LOGDIR_FAIRNESS/normal/
LOGMIXED=$LOGDIR_FAIRNESS/mixed/
SERVER="node@n2"
CLIENT="node@n1"
SERVER_IP=10.0.1.2
CLIENT_IP=10.0.1.1
CODED_SERVER_IP=10.1.1.2
DEV1=veth1
DEV2=veth2

# Coder directory on the virtual machines
CODERDIR=/home/node/share/tun/build/apps/singlepath/

# Coder logdir
CLOG=/home/node/share/logs/$LOGDIR_FAIRNESS

# Coder setup
KODO_REDUNDANCY="120 140 200 300"


MAXRATE=20mbit

#Measurement setup
DURATION=60

function poll_recovery_stats {
    if [ $# != 0 ]; then
        echo "poll_recovery_stats argument mismatch"
        exit 1
    fi
    ssh $SERVER "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
    ssh $CLIENT "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"

    sleep 1
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


    run_iperf $1 $2
}

function run_kodo {
    if [ $# != 3 ]; then
        echo "run_kodo argument mismatch. Arguments needed: \"coding_server_ip iperf_logfile redundancy\""
        exit 1
    fi

    if [ "$3" -lt "100" ]; then
        echo kodo redundancy below 100. Not allowed.
        exit 1
    fi
    ## Setup
    CODER=pc_kodo

    # echo running kodo
    tmux new-window -dk -t 4 "ssh $CLIENT \"$CODERDIR$CODER ncif $3 /dev/null $CLIENT_IP $SERVER_IP\""
    tmux new-window -dk -t 5 "ssh $SERVER \"$CODERDIR$CODER ncif $3 /dev/null $SERVER_IP $CLIENT_IP\""

    sleep 1

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall -q $CODER"
    ssh $SERVER "killall -q $CODER"
    sleep 1
}


#Make sure an iperf server is running on the server node

mkdir $LOGDIR_FAIRNESS
mkdir $LOGNORMAL
mkdir $LOGMIXED


# Cleaning up:
sudo tc qdisc del dev $DEV1 root
sudo tc qdisc del dev $DEV2 root

sudo tc qdisc del dev of_tap1 root
sudo tc qdisc del dev of_tap2 root
sudo tc qdisc add dev of_tap1 root netem rate $MAXRATE
sudo tc qdisc add dev of_tap2 root netem rate $MAXRATE

# Set client to run a series of tcp measurements
for i in $(seq 1 10); do
    for ERROR_RATE in 0 0.1 0.2 0.3 0.4 0.5 1 1.5 2 2.5 3 3.5 4 4.5 5 6 7 8 9 10
    do
        # Set delay and variance
        DELAY=10
        DELAY_VAR=$(expr $DELAY / 5)

        sudo tc qdisc add dev $DEV1 root netem \
            rate $MAXRATE delay $DELAY\ms $DELAY_VAR\ms distribution normal loss $ERROR_RATE

        sudo tc qdisc add dev $DEV2 root netem \
            rate $MAXRATE delay $DELAY\ms $DELAY_VAR\ms distribution normal loss $ERROR_RATE

        sleep 10

        echo Running two streams of normal TCP with $ERROR_RATE percent error

        LOGFILE_CLIENT1=iperf_fairness_1_normal_delay_$DELAY\_error_$ERROR_RATE\.dat
        LOGFILE_CLIENT2=iperf_fairness_2_normal_delay_$DELAY\_error_$ERROR_RATE\.dat

        run_uncoded $SERVER_IP $LOGNORMAL$LOGFILE_CLIENT1 & \
            run_uncoded $SERVER_IP $LOGNORMAL$LOGFILE_CLIENT2

        # ssh $CLIENT "iperf -c $SERVER_IP -t $DURATION -y C" >> $LOGNORMAL/$LOGFILE_CLIENT1 & ssh $CLIENT "iperf -c $SERVER_IP -t $DURATION -y C" >> $LOGNORMAL/$LOGFILE_CLIENT2



        for RED in $KODO_REDUNDANCY; do
            sleep 10

            echo Running mixed with kodo_$RED and $ERROR_RATE percent error

            LOGFILE_CODED=iperf_fairness_kodo_$RED\_delay_$DELAY\_error_$ERROR_RATE\.dat
            LOGFILE_NORMAL=iperf_fairness_normal_kodo_$RED\_delay_$DELAY\_error_$ERROR_RATE\.dat

            run_kodo $CODED_SERVER_IP $LOGMIXED$LOGFILE_CODED $RED & \
                run_uncoded $SERVER_IP $LOGMIXED$LOGFILE_NORMAL

        done

        # Remove loss on packets
        sudo tc qdisc del dev $DEV1 root
        sudo tc qdisc del dev $DEV2 root
    done
done
sudo tc qdisc del dev of_tap1 root
sudo tc qdisc del dev of_tap2 root
