#!/bin/bash
#enc: utf8

# General parameters
LOGDIR=../logs/
SERVER="node@n2"
CLIENT="node@n1"
RECODER="node@n3"
CLIENT_IP=10.0.1.1
SERVER_IP=10.0.1.2
RECODER_IP=10.0.1.3
CODED_SERVER_IP=10.1.1.2
DEV11=veth11
DEV12=veth12
DEV21=veth21
DEV22=veth22

MAXRATE=24mbit

# Iperf duration
DURATION=60

# Coder directory on the virtual machines
CODERDIR=/home/node/share/tun/build/apps/singlepath/

# Coder logdir
CLOG=/home/node/share/logs/

XOR_REDUNDANCY="3 5"
KODO_REDUNDANCY="120 140 200 300"

# Coders in use
# CODERS="pc_xor pc_kodo pc_kodo_xor"

function poll_recovery_stats {
    if [ $# != 0 ]; then
        echo "poll_recovery_stats argument mismatch"
        exit 1
    fi
    ssh $SERVER "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
    ssh $CLIENT "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"
    ssh $RECODER "echo \"print_decode_stat\" > /dev/udp/127.0.0.1/54321"

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

function run_xor {
    if [ $# != 4 ]; then
        echo "run_xorargument mismatch. Arguments needed: \"coding_server_ip iperf_logfile coding_logfile redundancy\""
        exit 1
    fi
    ## Setup
    CODER=pc_xor

    tmux new-window -dk -t 4 "ssh $CLIENT \"$CODERDIR$CODER ncif $4 /dev/null $CLIENT_IP $SERVER_IP\""
    tmux new-window -dk -t 5 "ssh $SERVER \"$CODERDIR$CODER ncif $4 $3 $SERVER_IP $CLIENT_IP\""

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall $CODER"
    ssh $SERVER "killall $CODER"
    sleep 1
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
    tmux new-window -dk -t 4 "ssh $CLIENT \"$CODERDIR$CODER ncif $4 /dev/null $CLIENT_IP $SERVER_IP\""
    tmux new-window -dk -t 5 "ssh $SERVER \"$CODERDIR$CODER ncif $4 $3 $SERVER_IP $CLIENT_IP\""

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall $CODER"
    ssh $SERVER "killall $CODER"
    sleep 1
}

function run_kodo_intermediate {
    if [ $# != 4 ]; then
        echo run_uncoded argument mismatch. Arguments needed: "coding_server_ip iperf_logfile coding_logfile redundancy"
        exit 1
    fi

    if [ $4 -lt 100 ]; then
        echo kodo redundancy below 100. Not allowed.
        exit 1
    fi
    ## Setup
    CODER=pc_kodo
    RCODER=pc_recoder

    tmux new-window -dk -t 4 \
        "ssh $CLIENT \"$CODERDIR$CODER ncif 100 /dev/null $CLIENT_IP $RECODER_IP 11111 11111\""
    tmux new-window -dk -t 5 \
        "ssh $RECODER \"$CODERDIR$RCODER 100 $RECODER_IP $CLIENT_IP 11111 11111 $4 $RECODER_IP $SERVER_IP 22222 22222\""
    tmux new-window -dk -t 6 \
        "ssh $SERVER \"$CODERDIR$CODER ncif $4 $3 $SERVER_IP $RECODER_IP 22222 22222\""

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall $CODER"
    ssh $SERVER "killall $CODER"
    ssh $RECODER "killall $RCODER"
}


if [ $TERM != "screen" ]; then
    echo Don\'t we want to be inside a screen?
    exit 1
fi


# Cleaning up:
sudo tc qdisc del dev $DEV11 root
sudo tc qdisc del dev $DEV12 root
sudo tc qdisc del dev $DEV21 root
sudo tc qdisc del dev $DEV22 root

sudo tc qdisc del dev of_tap1 root
sudo tc qdisc del dev of_tap2 root

sudo tc qdisc add dev of_tap1 root netem rate $MAXRATE
sudo tc qdisc add dev of_tap2 root netem rate $MAXRATE


for i in $(seq 1 10); do

    # Set delay on first hop (DEV11-DEV12)
    for DELAY1 in 25 50 100; do
        DELAY1_VAR=$(expr $DELAY1 / 5)

        sudo tc qdisc add dev $DEV11 root netem \
            rate $MAXRATE delay "$DELAY1"ms "$DELAY1_VAR"ms distribution normal

        sudo tc qdisc add dev $DEV12 root netem \
            rate $MAXRATE delay "$DELAY1"ms "$DELAY1_VAR"ms distribution normal


        for ERROR_RATE in 0 0.5 1 2 3 4 5 6 8 10
        do
            echo Setting error rate to $ERROR_RATE percent loss

            for DELAY in 5; do
                DELAY_VAR=$(expr $DELAY / 5)

                echo Setting delay to $DELAY milliseconds with variance "$DELAY_VAR" milliseconds

                sudo tc qdisc add dev $DEV21 root netem \
                    rate $MAXRATE delay "$DELAY"ms "$DELAY_VAR"ms distribution normal loss $ERROR_RATE

                sudo tc qdisc add dev $DEV22 root netem \
                    rate $MAXRATE delay "$DELAY"ms "$DELAY_VAR"ms distribution normal loss $ERROR_RATE

                echo Running uncoded iperf - $DURATION seconds
                IPERF_LOGFILE=iperf_multihop1_uncoded_delay_$DELAY1\_$DELAY\_error_$ERROR_RATE.dat
                run_uncoded $SERVER_IP $LOGDIR$IPERF_LOGFILE
                sleep 1

                for RED in $XOR_REDUNDANCY; do
                    echo Running xor iperf - $DURATION seconds
                    LOG=multihop1_xor_delay_$DELAY1\_$DELAY\_error_$ERROR_RATE\_redundancy_$RED.dat
                    IPERF_LOGFILE=iperf_$LOG
                    CODING_LOGFILE=recovery_$LOG
                    run_xor $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
                    sleep 1
                done

                for RED in $KODO_REDUNDANCY; do
                    echo Running kodo iperf - $DURATION seconds
                    LOG=multihop1_kodo_delay_$DELAY1\_$DELAY\_error_$ERROR_RATE\_redundancy_$RED.dat
                    IPERF_LOGFILE=iperf_$LOG
                    CODING_LOGFILE=recovery_$LOG
                    run_kodo $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
                    sleep 1
                done

                for RED in $KODO_REDUNDANCY; do
                    echo Running kodo intermediate iperf - $DURATION seconds
                    LOG=multihop1_kodo_intermediate_delay_$DELAY1\_$DELAY\_error_$ERROR_RATE\_redundancy_$RED.dat
                    IPERF_LOGFILE=iperf_$LOG
                    CODING_LOGFILE=recovery_$LOG
                    run_kodo_intermediate $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
                    sleep 1
                done

                sudo tc qdisc del dev $DEV21 root
                sudo tc qdisc del dev $DEV22 root

            done
        done
        sudo tc qdisc del dev $DEV11 root
        sudo tc qdisc del dev $DEV12 root
    done
done
