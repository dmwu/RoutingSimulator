#ifndef FAT_TREE
#define FAT_TREE

#include "../../main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "loggers.h"
#include "network.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include <ostream>

#define FAIL_RATE 0.0

class FatTreeTopology : public Topology {
public:
    Pipe *pipes_nc_nup[NC][NK];
    Pipe *pipes_nup_nlp[NK][NK];
    Pipe *pipes_nlp_ns[NK][NSRV];
    RandomQueue *queues_nc_nup[NC][NK];//[WDM] queue is on the second end of the link
    RandomQueue *queues_nup_nlp[NK][NK];
    // RandomQueue *queues_nlp_ns[NK][NSRV]; //handle host queues separately

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
    bool *_failedSwitches;
    int _numLinks;
    int _numSwitches;
    FatTreeTopology(EventList *ev);

    virtual void init_network();

    void count_queue(RandomQueue *);

    void print_path(std::ofstream &paths, int src, route_t *route);

    // [WDM]


    int rand_host_sw(int sw);

    int nodePair_to_link(int a, int b);

    virtual int getServerUnderTor(int tor, int torNum);

    virtual void failLink(int linkid);

    virtual void failSwitch(int sid);

    virtual void recoverLink(int linkid);

    virtual void recoverSwitch(int sid);

    virtual pair<route_t *, route_t *> getReroutingPath(int src, int dest, route_t* currentPath= nullptr);

    virtual pair<route_t *, route_t *> getStandardPath(int src, int dest);

    virtual pair<route_t*, route_t*> getEcmpPath(int src, int dest);

    virtual vector<int> *get_neighbours(int src) { return NULL; };

    virtual pair<Queue *, Queue *> linkToQueues(int linkid);

    virtual bool isPathValid(route_t *path);

    map<RandomQueue *, int> _link_usage;
    virtual pair<route_t*, route_t*> getLeastLoadedPath(int src, int dest);
    virtual pair<route_t*, route_t*> getOneWorkingPath(int src, int dest);
    vector<route_t *> ***_net_paths;

    int find_lp_switch(RandomQueue *queue);

    int find_up_switch(RandomQueue *queue);

    int find_core_switch(RandomQueue *queue);

    int find_destination(RandomQueue *queue);

    vector<route_t *> *get_paths_ecmp(int src, int dest);

    route_t *get_path_2levelrt(int src, int dest);

    route_t* getReversePath(int src, int dest, route_t*dataPath);

    virtual vector<int>* getLinksFromSwitch(int sid);
};

#endif
