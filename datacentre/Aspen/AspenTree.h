//
// Created by Dingming Wu on 1/20/18.
//

#ifndef SIM_ASPENTREE_H
#define SIM_ASPENTREE_H


#include <fat_tree_topology.h>

class AspenTree: public FatTreeTopology {
public:

    Pipe *pipes_nup_nred[NK][NK];
    Pipe *pipes_nred_nc[NK][NC];

    Pipe *pipes_nc_nred[NC][NK];
    Pipe *pipes_nred_nup[NK][NK];

    RandomQueue *queues_nup_nred[NK][NK];
    RandomQueue *queues_nred_nc[NK][NC];

    RandomQueue *queues_nc_nred[NC][NK];
    RandomQueue *queues_nred_nup[NK][NK];

    //the lower part of a aspen tree is extactly the same as a fattree,
    //we don't allow a failure happen in the links that are unique to aspenTree
    AspenTree(EventList* ev);

    virtual pair<route_t *, route_t *> getReroutingPath(int src, int dest, route_t* currentPath= nullptr);

    virtual pair<route_t *, route_t *> getStandardPath(int src, int dest);

    virtual pair<route_t*, route_t*> getEcmpPath(int src, int dest);
    route_t* inflatePath(route_t*path, int src, int dest);

    vector<route_t *>*  get_paths_ecmp(int src, int dest);


};


#endif //SIM_ASPENTREE_H
