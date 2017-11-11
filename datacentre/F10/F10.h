//
// Created by Dingming Wu on 11/8/17.
//

#ifndef SIM_F10_H
#define SIM_F10_H
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "network.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include "../../main.h"
#include <ostream>
#include <topology.h>

#define FAIL_RATE 0.0

#define NK (K*K/2)
#define NC (K*K/4)
#define NSRV (K*K*K*RATIO/4)

#define HOST_POD_SWITCH(src) (2*src/(K*RATIO))
#define HOST_POD(src) (src/(NC*RATIO))

#define MIN_POD_ID(pod_id) (pod_id*K/2)
#define MAX_POD_ID(pod_id) ((pod_id+1)*K/2-1)

class F10Topology: public Topology {

public:

    F10Topology(EventList *ev);
    Pipe *pipes_nc_nup[NC][NK];
    Pipe *pipes_nup_nlp[NK][NK];
    Pipe *pipes_nlp_ns[NK][NSRV];
    RandomQueue *queues_nc_nup[NC][NK];//[WDM] queue is on the second end of the link
    RandomQueue *queues_nup_nlp[NK][NK];

    Pipe *pipes_nup_nc[NK][NC];
    Pipe *pipes_nlp_nup[NK][NK];
    Pipe *pipes_ns_nlp[NSRV][NK];
    RandomQueue *queues_nup_nc[NK][NC];
    RandomQueue *queues_nlp_nup[NK][NK];
    RandomQueue *queues_ns_nlp[NSRV][NK];
    RandomQueue *HostRecvQueues[NSRV];
    RandomQueue *HostTXQueues[NSRV];

    EventList *_eventlist;

    bool *_failedLinks;
    int _numLinks;

    virtual void init_network();

    int rand_host_sw(int sw);

    int nodePair_to_link(int a, int b);

    virtual int getServerUnderTor(int tor, int torNum);

    virtual void failLink(int linkid);

    virtual void recoverLink(int linkid);

    virtual pair<route_t *, route_t *> getReroutingPath(int src, int dest, route_t*);

    virtual pair<route_t*, route_t*> getStandardPath(int src, int dest);

    virtual vector<int> *get_neighbours(int src) { return NULL; };

    virtual pair<Queue *, Queue *> linkToQueues(int linkid);

    virtual void printPath(std::ostream &out, route_t *route);


protected:

    vector<route_t *> ***_net_paths;

    vector<route_t *> *get_paths_ecmp(int src, int dest);

    route_t *getAlternativePath(int src, int dest, route_t*path);

    route_t *get_path_2levelrt(int src, int dest);

    bool isPathValid(route_t *path);


};


#endif //SIM_F10_H
