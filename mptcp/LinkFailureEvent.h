//
// Created by Dingming Wu on 10/31/17.
//

#ifndef SIM_RUNTIMELINKFAILURE_H
#define SIM_RUNTIMELINKFAILURE_H


#include "eventlist.h"
#include "topology.h"
#include "tcp.h"
#include "queue.h"
#include "../../main.h"

typedef vector<PacketSink*> route_t;
class FlowConnection{
public:
    FlowConnection(TcpSrc* tcpSrc, TcpSink* tcpSink, int superID, int src, int dest, uint32_t fs, uint32_t arrivalTime_ms);
    FlowConnection(TcpSrc*, TcpSink*, int, int);
    TcpSrc* _tcpSrc;
    TcpSink* _tcpSink;
    int _src;
    int _dest;
    int _superId;
    uint32_t _flowSize;
    uint32_t _arrivalTimeMs;
    uint32_t _completionTimeMs=-1;
    uint32_t _duration=-1;

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
    double getThroughputOfImpactedFlows(map<int,FlowConnection*>* flowStats);
    void doNextEvent();
    simtime_picosec _startFrom;
    simtime_picosec _failureTime;
    Topology* _topo;
    LinkState _linkStatus = GOOD;
    bool UsingShareBackup = false;
    int _linkid;
    simtime_picosec _setupReroutingDelay, _pathRestoreDelay;
    vector<FlowConnection*>* _connections;
    void registerConnection(FlowConnection* fc);
    bool isPathOverlapping(route_t*);
    vector<int>* lpBackupUsageTracker;
    vector<int>* upBackupUsageTracker;
    vector<int>* coreBackupUsageTracker;
    int* _group1;
    int* _group2;
    void setBackupUsageTracker(vector<int>* lp, vector<int>*up, vector<int>* core);
private:
    void rerouting();
    bool hasEnoughBackup();
    void initTracker();
    void circuitReconfig();
};


#endif //SIM_RUNTIMELINKFAILURE_H
