#ifndef TOPOLOGY
#define TOPOLOGY

#include <sstream>
#include "network.h"

typedef vector<PacketSink *> route_t;

class Topology {
public:

    virtual void failLink(int linkid) = 0;

    virtual int getServerUnderTor(int tor, int torNum) = 0;

    virtual void recoverLink(int linkid) = 0;

    virtual pair<route_t*, route_t*> getReroutingPath(int src, int dest, route_t* currrentPath) = 0;


    virtual pair<route_t*, route_t*> getStandardPath(int src, int dest) = 0;


    virtual pair<Queue*, Queue*> linkToQueues(int linkid) = 0;

    virtual vector<int>* get_neighbours(int src) = 0;

    virtual void printPath(std::ostream& out, route_t *route) = 0;

    int RTT = 1; // Identical RTT microseconds = 0.001 ms [WDM] I change it to 1us to match the link speed.

    string ntoa(double n) {
        stringstream s;
        s << n;
        return s.str();
    }

    string itoa(uint64_t n) {
        stringstream s;
        s << n;
        return s.str();
    }

};

#endif
