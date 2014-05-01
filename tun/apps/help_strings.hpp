#pragma once

#define TUNIF_HELP_STRING "Name of the tun interface, e.g., tun0"
#define KODO_REDUNDANCY_FACTOR_HELP_STRING "The amount of redundancy in percent [100 - inf)"
#define DECODE_STAT_FILE_HELP_STRING "Path to stat file, stats are saved to this file. Can be set to /dev/null"
#define LOCAL_IP_HELP_STRING "IP address of the local physical interface to use, e.g., IP of eth0 or wlan0"
#define REMOTE_IP_HELP_STRING "IP address of the remote physical interface to use, e.g., IP of eth0 or wlan0 on remote host"
#define LOCAL_PORT_HELP_STRING "Local UDP port used to send and receive packets"
#define REMOTE_PORT_HELP_STRING "Remote UDP port used to send and receive packets"
#define XOR_AMOUNT_HELP_STRING "Amount of packets included in an XOR packet. 0 = 0 % redundancy, 1 = 100 % (repetition code), 2 = 50 %"
#define PROBABILITY_HELP_STRING "The probability in percent [1 - 100) of sending packets through \"this\" connection. The sum must be equal to 100"
