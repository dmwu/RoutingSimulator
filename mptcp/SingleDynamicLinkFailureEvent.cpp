//
// Created by Dingming Wu on 10/31/17.
//

#include "SingleDynamicLinkFailureEvent.h"
#include <iostream>
#include "tcp.h"
#include <set>

using namespace std;

SingleDynamicLinkFailureEvent::SingleDynamicLinkFailureEvent(EventList &eventlist, Topology *topo, simtime_picosec startFrom,
                                   simtime_picosec failureTime, int linkid)
        : EventSource(eventlist, "SingleDynamicLinkFailureEvent"), _startFrom(startFrom),
          _failureTime(failureTime), _topo(topo), _linkid(linkid) {
    _connections = new set<TcpSrc *>();
}

SingleDynamicLinkFailureEvent::SingleDynamicLinkFailureEvent(EventList &eventlist, Topology *topo)
        : EventSource(eventlist, "SingleDynamicLinkFailureEvent"), _topo(topo) {
    _connections = new set<TcpSrc *>();
}

SingleDynamicLinkFailureEvent::SingleDynamicLinkFailureEvent(EventList &eventList, simtime_picosec startFrom, simtime_picosec failureTime,
                                   int linkid):EventSource(eventList,"SingleDynamicLinkFailureEvent"), _startFrom(startFrom),
                                               _failureTime(failureTime), _linkid(linkid) {
    _connections = new set<TcpSrc*>();
}

void SingleDynamicLinkFailureEvent::setBackupUsageTracker(vector<int> *lp, vector<int> *up, vector<int> *core) {
    lpBackupUsageTracker = lp;
    upBackupUsageTracker = up;
    coreBackupUsageTracker = core;
    initTracker();
}
void SingleDynamicLinkFailureEvent::initTracker() {
    if(_linkid < 0)
        return;
    pair<Queue*,Queue*> ret = _topo->linkToQueues(_linkid);
    int sid2 = ret.first->_switchId; //upper queue
    int sid1 = ret.second->_switchId;//lower queue
    assert(sid1<=sid2);
    if(sid1 < 0){ //host link failure
        int gid2 = sid2/(K/2);
        _group1 = &(lpBackupUsageTracker->at(gid2));
        _group2 = &(lpBackupUsageTracker->at(gid2));
    }
    else if(sid1<NK) {
        //lp to up link failure
        assert(sid2 >= NK && sid2 < 2 * NK);
        int gid1 = sid1/(K/2);
        int gid2 = (sid2 -NK)/(K/2);
        _group1 = &(lpBackupUsageTracker->at(gid1));
        _group2 = &(upBackupUsageTracker->at(gid2));

    }else{
        // up to core link failure
        assert(sid1<2*NK && sid2>=2*NK);
        int gid1 = (sid1-NK)/(K/2);
        int gid2 = (sid2 -2*NK)/(K/2);
        _group1 = &(upBackupUsageTracker->at(gid1));
        _group2 = &(coreBackupUsageTracker->at(gid2));
    }
}

void SingleDynamicLinkFailureEvent::setFailedLinkid(int linkid) {
    _linkid = linkid;
}

void SingleDynamicLinkFailureEvent::setTopology(Topology *topo) {
    _topo = topo;
}
void SingleDynamicLinkFailureEvent::setStartEndTime(simtime_picosec startFrom, simtime_picosec failureTime) {
    _startFrom = startFrom;
    _failureTime = failureTime;
}

void SingleDynamicLinkFailureEvent::installEvent() {
    this->eventlist().sourceIsPending(*this, _startFrom);
}

void SingleDynamicLinkFailureEvent::setFailureRecoveryDelay(simtime_picosec setupReroutingDelay, simtime_picosec pathRestoreDelay) {
    _setupReroutingDelay = setupReroutingDelay;
    _pathRestoreDelay = pathRestoreDelay;
}

void SingleDynamicLinkFailureEvent::registerConnection(TcpSrc* tcpSrc) {
    _connections->insert(tcpSrc);
}

void SingleDynamicLinkFailureEvent::removeConnection(TcpSrc* tcpSrc) {

    if(_connections->count(tcpSrc)>0)
        _connections->erase(tcpSrc);
}

void SingleDynamicLinkFailureEvent::doNextEvent() {
    if(UsingShareBackup && hasEnoughBackup() )
        circuitReconfig();
    else
        rerouting();

}

void SingleDynamicLinkFailureEvent::circuitReconfig() {
    if (_linkStatus == GOOD) {
        _topo->failLink(_linkid);
        this->eventlist().sourceIsPending(*this, eventlist().now() + _setupReroutingDelay);
        (*_group1)--;
        (*_group2)--;
        _linkStatus = WAITING_REROUTING;
    }else if(_linkStatus == WAITING_REROUTING){
        _topo->recoverLink(_linkid);
        _linkStatus = BAD;
        this->eventlist().sourceIsPending(*this, eventlist().now() + _failureTime);
    }else if(_linkStatus == BAD){;
        (*_group1)++;
        (*_group2)++;
        _linkStatus = GOOD;
    }
}

bool SingleDynamicLinkFailureEvent::hasEnoughBackup() {
    return *_group1 >0 && *_group2 >0;
}

void SingleDynamicLinkFailureEvent::rerouting(){
    if (_linkStatus == GOOD) {
        _topo->failLink(_linkid);
        this->eventlist().sourceIsPending(*this, eventlist().now() + _setupReroutingDelay);
        _linkStatus = WAITING_REROUTING;

    } else if (_linkStatus == WAITING_REROUTING) {
        set<TcpSrc*>* copyOfConnections = new set<TcpSrc*>(*_connections);
        for (TcpSrc* tcp: *copyOfConnections) {
            if((tcp->_flow_finish)){
                continue;
            }

            tcp->handleImpactedFlow();

            route_t*currentPath = new route_t(tcp->_route->begin(), tcp->_route->end()-1);
            pair<route_t *, route_t *> newDataPath = _topo->getReroutingPath(tcp->_src, tcp->_dest, currentPath);
            if (_topo->isPathValid(newDataPath.first) && _topo->isPathValid(newDataPath.second)) {
                cout << "Now:"<<eventlist().now()/1e9<<" route change for flow " << tcp->_src << "->"
                     << tcp->_dest <<" timeout:"<<tcp->_rto/1e9<<"ms"<<endl;
                cout << "[old path]---- ";
                _topo->printPath(cout, tcp->_route);
                tcp->replace_route(newDataPath.first);
                tcp->_sink->replace_route(newDataPath.second);
                cout << "[new path]---- ";
                _topo->printPath(cout, tcp->_route);
                tcp->handleFlowRerouting(newDataPath.first);

            } else {
                tcp->handleFlowDeath();
                cout << "Now:"<<eventlist().now()/1e9
                     << " link failure:"<<_linkid<< " no path in rerouting for flow " << tcp->_src << "->" << tcp->_dest << endl;
            }
        }
        _linkStatus = BAD;
        this->eventlist().sourceIsPending(*this, eventlist().now() + _failureTime);

    } else if (_linkStatus == BAD) {
        this->eventlist().sourceIsPending(*this, eventlist().now() + _pathRestoreDelay);
        _linkStatus = WAITING_RECOVER;


    } else {
        _topo->recoverLink(_linkid);
        pair<route_t *, route_t *> path;
        for (TcpSrc *tcp: *_connections) {
            if((tcp->_flow_finish)){
                continue;
            }
            path = _topo->getStandardPath(tcp->_src, tcp->_dest);
            if (path.first == NULL || path.second == NULL) {
                route_t*currentPath = new route_t(tcp->_route->begin(), tcp->_route->end()-1);
                path = _topo->getReroutingPath(tcp->_src, tcp->_dest, currentPath);
                currentPath->clear();
            }
            if (_topo->isPathValid(path.first) && _topo->isPathValid(path.second)) {
                cout << "Now:"<<eventlist().now()/1e9
                     << "route [restore/reroute] for flow " << tcp->_src << "->" << tcp->_dest << endl;
                cout << "[rerouted path]---- ";
                _topo->printPath(cout, tcp->_route);
                tcp->replace_route(path.first);
                tcp->_sink->replace_route(path.second);
                cout << "[original path]----";
                _topo->printPath(cout, tcp->_route);
            } else {
                cout << "Now:"<<eventlist().now()/1e9
                     <<" link failure:"<<_linkid<<" no path in failure recovery for flow " << tcp->_src << "->" << tcp->_dest << endl;
            }
        }
        _linkStatus = GOOD;
    }
}



bool SingleDynamicLinkFailureEvent::isPathOverlapping(route_t *path) {
    pair<Queue *, Queue *> queues = _topo->linkToQueues(_linkid);
    Queue *first = queues.first;
    Queue *second = queues.second;
    for (PacketSink *t: *path) {
        if (t->_gid == first->_gid || t->_gid == second->_gid) {
            return true;
        }
    }
    return false;
}
