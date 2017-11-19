#ifndef MAIN_H
#define MAIN_H

#include <string>

#define REROUTE 0

// RRG STUFF
#define R 0		// R = switch-switch ports (or network-ports)
#define SP 0		// P = total ports on switch
#define RTT 10 // Identical RTT microseconds = 0.01 ms [WDM] I change it to 100us to match the link speed.
// FAT
#define K 16
#define SERVER_LEVEL_TRAFFIC 1
#define RATIO 1 //MUST BE 1 IF USING 2-LEVEL ROUTING
#define NSW K*K*5/4
#define NHOST (K*K*K*RATIO/4)

#define NK (K*K/2)
#define NC (K*K/4)
#define NSRV (K*K*K*RATIO/4)

#define HOST_POD_SWITCH(src) (2*src/(K*RATIO))
#define HOST_POD(src) (src/(NC*RATIO))
#define MIN_POD_ID(pod_id) (pod_id*K/2)
#define MAX_POD_ID(pod_id) ((pod_id+1)*K/2-1)

#define SW_BW 125000 // switch link bandwidth in pps = 1Gbps
#define HOST_NIC SW_BW // host nic speed in pps
#define CORE_TO_HOST 1 // right now it is non-blocking


#define SWITCH_BUFFER 10
#define RANDOM_BUFFER 0
#define FEEDER_BUFFER 100000 //100MB, feeder queue shouldn't be the bottleneck

#define BACKUPS_PER_GROUP 1

#define LOCAL_REROUTE_DELAY 2 //ms
#define GLOBAL_REROUTE_DELAY 30 //ms
#define CIRCUIT_SWITCHING_DELAY 10 //ms
#define TCP_TIMEOUT_SCANNER_PERIOD 0.7 //ms

#endif

