#ifndef TOPOLOGY
#define TOPOLOGY

#include <sstream>
#include "network.h"
#include "queue.h"
typedef vector<PacketSink *> route_t;

class Topology {
public:

    virtual void failLink(int linkid) = 0;

    virtual int getServerUnderTor(int tor, int torNum) = 0;

    virtual void recoverLink(int linkid) = 0;

    virtual void failSwitch(int sid) =0;

    virtual void recoverSwitch(int sid) =0;

    virtual pair<route_t*, route_t*> getReroutingPath(int src, int dest, route_t* currrentPath) = 0;

    virtual pair<route_t*, route_t*> getEcmpPath(int src, int dest) = 0;
    virtual bool isPathValid(route_t *path) =0;

    virtual pair<route_t*, route_t*> getStandardPath(int src, int dest) = 0;

    virtual pair<Queue*, Queue*> linkToQueues(int linkid) = 0;

    virtual vector<int>* get_neighbours(int src) = 0;

    static void printPath(std::ostream& out, route_t *rt){
        for (unsigned int i = 0; i < rt->size(); i += 2) {
            Queue *q = (Queue *) rt->at(i);
            if (q != NULL)
                out << q->_gid << " ";
            else
                out << "NULL ";
        }
        out << endl;
    }


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
