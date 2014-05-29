#!/bin/bash
#enc: utf8

# General parameters
LOGDIR=logs/
SERVER="node@n2"
CLIENT="node@n1"
CLIENT_IP=10.0.1.1
SERVER_IP=10.0.1.2
CODED_SERVER_IP=10.1.1.2
DEV1=veth1
DEV2=veth2

MAXRATE=20mbit

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

function run_kodo_xor {
    if [ $# != 5 ]; then
        echo run_uncoded argument mismatch. Arguments needed: "coding_server_ip iperf_logfile coding_logfile redundancy xor_amount"
        exit 1
    fi

    if [ $4 -lt 100 ]; then
        echo kodo redundancy below 100. Not allowed.
        exit 1
    fi
    ## Setup
    CODER=pc_kodo_xor

    tmux new-window -dk -t 4 "ssh $CLIENT \"$CODERDIR$CODER ncif $4 $5 /dev/null $CLIENT_IP $SERVER_IP\""
    tmux new-window -dk -t 5 "ssh $SERVER \"$CODERDIR$CODER ncif $4 $5 $3 $SERVER_IP $CLIENT_IP\""
    # tmux new-window -dk -t 6 "ssh $SERVER \"iperf -s\""

    run_iperf $1 $2
    poll_recovery_stats

    ssh $CLIENT "killall $CODER"
    ssh $SERVER "killall $CODER"
    sleep 1

}



if [ $TERM != "screen" ]; then
    echo Don\'t we want to be inside a screen?
    exit 1
fi

# Cleaning up:
sudo tc qdisc del dev $DEV1 root
sudo tc qdisc del dev $DEV2 root

sudo tc qdisc del dev of_tap1 root
sudo tc qdisc del dev of_tap2 root
sudo tc qdisc add dev of_tap1 root netem rate $MAXRATE
sudo tc qdisc add dev of_tap2 root netem rate $MAXRATE

#Make sure an iperf server is running on the server node

for i in $(seq 1 10); do
    for ERROR_RATE in 0 0.5 1 2 3 4 5 6 8 10
    do
        echo Setting error rate to $ERROR_RATE percent loss

        # Set error and maxrate
        sudo tc qdisc add dev $DEV1 root netem rate $MAXRATE loss $ERROR_RATE
        sudo tc qdisc add dev $DEV2 root netem rate $MAXRATE loss $ERROR_RATE

        # Run without delay:
        echo Running uncoded iperf - $DURATION seconds
        IPERF_LOGFILE=iperf_singlehop_uncoded_delay_0_error_$ERROR_RATE.dat
        run_uncoded $SERVER_IP $LOGDIR$IPERF_LOGFILE

        sleep 1

        for RED in $XOR_REDUNDANCY; do
            echo Running xor iperf - $DURATION seconds
            LOG=singlehop_xor_delay_0_error_$ERROR_RATE\_redundancy_$RED.dat
            IPERF_LOGFILE=iperf_$LOG
            CODING_LOGFILE=recovery_$LOG
            run_xor $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
            sleep 1
        done

        for RED in $KODO_REDUNDANCY; do
            echo Running kodo iperf - $DURATION seconds
            LOG=singlehop_kodo_delay_0_error_$ERROR_RATE\_redundancy_$RED.dat
            IPERF_LOGFILE=iperf_$LOG
            CODING_LOGFILE=recovery_$LOG
            run_kodo $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
            sleep 1
        done

        for kRED in 140; do
            for xRED in 0 5; do
                echo Running kodo_xor iperf - $DURATION seconds
                LOG=singlehop_kodo_xor_delay_0_error_$ERROR_RATE\_redundancy_$kRED\_$xRED.dat
                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG
                run_kodo_xor $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $kRED $xRED
                sleep 1
            done
        done

        # Remove loss on packets
        # echo Removing delay rule
        sudo tc qdisc del dev $DEV1 root
        sudo tc qdisc del dev $DEV2 root

        for DELAY in 10 50; do
            DELAY_VAR=$(expr $DELAY / 5)

            echo Setting delay to $DELAY milliseconds with variance "$DELAY_VAR" milliseconds

            sudo tc qdisc add dev $DEV1 root netem \
                rate $MAXRATE delay "$DELAY"ms "$DELAY_VAR"ms distribution normal loss $ERROR_RATE

            sudo tc qdisc add dev $DEV2 root netem \
                rate $MAXRATE delay "$DELAY"ms "$DELAY_VAR"ms distribution normal loss $ERROR_RATE

            echo Running uncoded iperf - $DURATION seconds
            IPERF_LOGFILE=iperf_singlehop_uncoded_delay_$DELAY\_error_$ERROR_RATE.dat
            run_uncoded $SERVER_IP $LOGDIR$IPERF_LOGFILE
            sleep 1

            for RED in $XOR_REDUNDANCY; do
                echo Running xor iperf - $DURATION seconds
                LOG=singlehop_xor_delay_$DELAY\_error_$ERROR_RATE\_redundancy_$RED.dat
                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG
                run_xor $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
                sleep 1
            done

            for RED in $KODO_REDUNDANCY; do
                echo Running kodo iperf - $DURATION seconds
                LOG=singlehop_kodo_delay_$DELAY\_error_$ERROR_RATE\_redundancy_$RED.dat
                IPERF_LOGFILE=iperf_$LOG
                CODING_LOGFILE=recovery_$LOG
                run_kodo $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $RED
                sleep 1
            done

            for kRED in 140; do
                for xRED in 0 5; do
                    echo Running kodo_xor iperf - $DURATION seconds
                    LOG=singlehop_kodo_xor_delay_$DELAY\_error_$ERROR_RATE\_redundancy_$kRED\_$xRED.dat
                    IPERF_LOGFILE=iperf_$LOG
                    CODING_LOGFILE=recovery_$LOG
                    run_kodo_xor $CODED_SERVER_IP $LOGDIR$IPERF_LOGFILE $CLOG$CODING_LOGFILE $kRED $xRED
                    sleep 1
                done
            done

            sudo tc qdisc del dev $DEV1 root
            sudo tc qdisc del dev $DEV2 root

        done
    done
done
