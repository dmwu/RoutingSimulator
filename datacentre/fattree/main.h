#ifndef MAIN_H
#define MAIN_H

#include <string>

#define REROUTE 0

// RRG STUFF
#define R 0		// R = switch-switch ports (or network-ports)
#define SP 0		// P = total ports on switch

// FAT
#define K 8
#define SERVER_LEVEL_TRAFFIC 1
#define RATIO 1 //MUST BE 1 IF USING 2-LEVEL ROUTING
#define NSW K*K*5/4
#define NHOST (K*K*K*RATIO/4)

#define SW_BW 1250000 // switch link bandwidth in pps = 10Gbps
#define HOST_NIC SW_BW // host nic speed in pps
#define CORE_TO_HOST 1 // right now it is non-blocking
#define NUMPATHS 1

// TOPOLOGY NUMBERING
#define FAT 0
#define RRG 1

// CHOSEN TOPOLOGY
#define CHOSEN_TOPO FAT

//basic setup!

#define NI 2        //Number of intermediate switches
#define NA 6        //Number of aggregation switches
#define NT 9        //Number of ToR switches (180 hosts)

#define NS 20        //Number of servers per ToR switch
#define TOR_AGG2(tor) (10*NA - tor - 1)%NA

/*
#define NI 4        //Number of intermediate switches
#define NA 9        //Number of aggregation switches
#define NT 18        //Number of ToR switches (180 hosts)

#define NS 10        //Number of servers per ToR switch
#define TOR_AGG2(tor) (tor+1)%NA
*/
/*
#define NI 10        //Number of intermediate switches
#define NA 6        //Number of aggregation switches
#define NT 30        //Number of ToR switches (180 hosts)

#define NS 6        //Number of servers per ToR switch
#define TOR_AGG2(tor) (tor+1)%NA
*/

#define NT2A 2      //Number of connections from a ToR to aggregation switches

#define TOR_ID(id) NSW+id
#define AGG_ID(id) NSW+NT+id
#define INT_ID(id) NSW+NT+NA+id
#define HOST_ID(hid,tid) tid*NS+hid

#define HOST_TOR(host) host/NS
#define HOST_TOR_ID(host) host%NS
#define TOR_AGG1(tor) tor%NA


#define SWITCH_BUFFER 100
#define RANDOM_BUFFER 2
#define FEEDER_BUFFER 1000

#endif
