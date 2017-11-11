//
// Created by Dingming Wu on 10/31/17.
//

#include "LinkFailureEvent.h"
#include <iostream>

using namespace std;

LinkFailureEvent::LinkFailureEvent(EventList &eventlist, Topology *topo, simtime_picosec startFrom,
                                   simtime_picosec failureTime, int linkid)
        : EventSource(eventlist, "LinkFailureEvent"), _topo(topo), _startFrom(startFrom),
          _failureTime(failureTime), _linkid(linkid) {
    _connections = new vector<FlowConnection *>();
}

LinkFailureEvent::LinkFailureEvent(EventList &eventlist, Topology *topo)
        : EventSource(eventlist, "LinkFailureEvent"), _topo(topo) {
    _connections = new vector<FlowConnection *>();
}

void LinkFailureEvent::setFailedLinkid(int linkid) {
    _linkid = linkid;
}

void LinkFailureEvent::setStartEndTime(simtime_picosec startFrom, simtime_picosec failureTime) {
    _startFrom = startFrom;
    _failureTime = failureTime;
}

void LinkFailureEvent::installEvent() {
    this->eventlist().sourceIsPending(*this, _startFrom);
}

void LinkFailureEvent::setFailureRecoveryDelay(simtime_picosec setupReroutingDelay, simtime_picosec pathRestoreDelay) {
    _setupReroutingDelay = setupReroutingDelay;
    _pathRestoreDelay = pathRestoreDelay;
}

void LinkFailureEvent::registerConnection(FlowConnection *fc) {
    _connections->push_back(fc);
}

void LinkFailureEvent::doNextEvent() {

    if (_linkStatus == GOOD) {
        _topo->failLink(_linkid);
        this->eventlist().sourceIsPending(*this, eventlist().now() + _setupReroutingDelay);
        _linkStatus = WAITING_REROUTING;

    } else if (_linkStatus == WAITING_REROUTING) {
        for (FlowConnection *fc: *_connections) {
            if(*(fc->_tcpSrc->_flow_finish)){
                continue;
            }
            route_t*currentPath = new route_t(fc->_tcpSrc->_route->begin(), fc->_tcpSrc->_route->end()-1);
            pair<route_t *, route_t *> newDataPath = _topo->getReroutingPath(fc->_src, fc->_dest, currentPath);
            if (newDataPath.first && newDataPath.second) {
                cout << "route change for flow " << fc->_src << "->" << fc->_dest << endl;
                cout << "[old path]---- ";
                _topo->printPath(cout, fc->_tcpSrc->_route);
                fc->_tcpSrc->replace_route(newDataPath.first);
                fc->_tcpSink->replace_route(newDataPath.second);
                cout << "[new path]---- ";
                _topo->printPath(cout, fc->_tcpSrc->_route);

            } else {
                cout << "link failure:"<<_linkid<< " no path in rerouting for flow " << fc->_src << "->" << fc->_dest << endl;
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
        for (FlowConnection *fc: *_connections) {
            if(*(fc->_tcpSrc->_flow_finish)){
                continue;
            }
            path = _topo->getStandardPath(fc->_src, fc->_dest);
            if (path.first == nullptr || path.second == nullptr) {
                route_t*currentPath = new route_t(fc->_tcpSrc->_route->begin(), fc->_tcpSrc->_route->end()-1);
                path = _topo->getReroutingPath(fc->_src, fc->_dest, currentPath);
                currentPath->clear();
            }
            if (path.first && path.second) {
                cout << "route [restore/reroute] for flow " << fc->_src << "->" << fc->_dest << endl;
                cout << "[old path]---- ";
                _topo->printPath(cout, fc->_tcpSrc->_route);
                fc->_tcpSrc->replace_route(path.first);
                fc->_tcpSink->replace_route(path.second);
                cout << "[new path]----";
                _topo->printPath(cout, fc->_tcpSrc->_route);
            } else {
                cout << "link failure:"<<_linkid<<" no path in failure recovery for flow " << fc->_src << "->" << fc->_dest << endl;
            }
        }
        _linkStatus = GOOD;
    }
}



bool LinkFailureEvent::isPathOverlapping(route_t *path) {
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

FlowConnection::FlowConnection(TcpSrc *tcpSrc, TcpSink *tcpSink, int src, int dest)
        : _tcpSrc(tcpSrc), _tcpSink(tcpSink), _src(src), _dest(dest) {}