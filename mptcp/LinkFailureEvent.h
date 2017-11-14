//
// Created by Dingming Wu on 10/31/17.
//

#ifndef SIM_RUNTIMELINKFAILURE_H
#define SIM_RUNTIMELINKFAILURE_H


#include "eventlist.h"
#include "topology.h"
#include "tcp.h"
#include "queue.h"

typedef vector<PacketSink*> route_t;
class FlowConnection{
public:
    FlowConnection(TcpSrc* tcpSrc, TcpSink* tcpSink, int src, int dest);
    TcpSrc* _tcpSrc;
    TcpSink* _tcpSink;
    int _src;
    int _dest;
};
enum LinkState {GOOD, WAITING_REROUTING, BAD, WAITING_RECOVER};

class LinkFailureEvent: public EventSource {
public:
    LinkFailureEvent(EventList& eventlist, Topology* topo,  simtime_picosec startFrom, simtime_picosec failureTime, int linkid);
    LinkFailureEvent(EventList& eventlist, Topology* topo);
    LinkFailureEvent(EventList& eventList, simtime_picosec startFrom, simtime_picosec failureTime, int linkid);
    void setStartEndTime(simtime_picosec startFrom, simtime_picosec endTime);
    void setFailedLinkid(int linkid);
    void installEvent();
    void setFailureRecoveryDelay(simtime_picosec setupReroutingDelay, simtime_picosec pathRestoreDelay);
    void setTopology(Topology*);
    double getThroughputOfImpactedFlows(map<int,double>* flowStats);
    void doNextEvent();
    simtime_picosec _startFrom;
    simtime_picosec _failureTime;
    Topology* _topo;
    LinkState _linkStatus = GOOD;
    int _linkid;
    simtime_picosec _setupReroutingDelay, _pathRestoreDelay;
    vector<FlowConnection*>* _connections;
    void registerConnection(FlowConnection* fc);
    bool isPathOverlapping(route_t*);
};


#endif //SIM_RUNTIMELINKFAILURE_H
