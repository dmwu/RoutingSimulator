//
// Created by Dingming Wu on 1/20/18.
//

#include "AspenTree.h"

AspenTree::AspenTree(EventList *ev):FatTreeTopology(ev) {

    // red layer to core!
    for (int ri = 0; ri < NK; ri++) {

        for (int l = 0; l < K / 2; l++) {
            int k = ri % (K / 2) + l*K/2;

            queues_nred_nc[ri][k] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                  memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                  *_eventlist, NULL,
                                                  memFromPkt(RANDOM_BUFFER),
                                                  "Queue-red-co-" + ntoa(ri) + "-" + ntoa(k), k + 3*NK);

            pipes_nred_nc[ri][k] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-red-co-" + ntoa(ri) + "-" + ntoa(k));


            queues_nc_nred[k][ri] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                  memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                  *_eventlist, NULL,
                                                  memFromPkt(RANDOM_BUFFER),
                                                  "Queue-co-red-" + ntoa(k) + "-" + ntoa(ri), ri + 2*NK);


            pipes_nc_nred[k][ri] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-co-red-" + ntoa(k) + "-" + ntoa(ri));

            queues_nred_nc[ri][k]->setDualQueue(queues_nc_nred[k][ri]);

            pipes_nc_nred[k][ri]->setDualPipe(pipes_nred_nc[ri][k]);

        }
    }

    //upper layer to red layer
    for (int j = 0; j < NK; j++) {
        int redPodIndex = j/(NK/2);
        int upPodOffset = j %(K/2);
        for (int i = 0; i < K/2; i++) {
            // uplink
            int k = redPodIndex*(NK/2)+upPodOffset*(K/2) + i;
            queues_nup_nred[j][k] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                   memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                   *_eventlist, NULL,
                                                   memFromPkt(RANDOM_BUFFER),
                                                   "Queue-up-red-" + ntoa(j) + "-" + ntoa(k), k+2*NK);

            pipes_nup_nred[j][k] = new Pipe(timeFromUs(RTT), *_eventlist,
                                           "Pipe-up-red-" + ntoa(j) + "-" + ntoa(k));

            // downlink
            queues_nred_nup[k][j] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                   memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                   *_eventlist, NULL,
                                                   memFromPkt(RANDOM_BUFFER),
                                                   "Queue-red-up-" + ntoa(k) + "-" + ntoa(j), j + NK);

            pipes_nred_nup[k][j] = new Pipe(timeFromUs(RTT), *_eventlist,
                                           "Pipe-red-up-" + ntoa(k) + "-" + ntoa(j));

            pipes_nred_nup[k][j]->setDualPipe(pipes_nup_nred[j][k]);
            queues_nred_nup[k][j]->setDualQueue(queues_nup_nred[j][k]);
        }
    }

};
pair<route_t*, route_t*> AspenTree::getEcmpPath(int src, int dest) {
    if (_net_paths[src][dest] == NULL) {
        _net_paths[src][dest] = get_paths_ecmp(src, dest);
    }
    vector<route_t *> *paths = _net_paths[src][dest];
    unsigned rnd = rand() % paths->size();
    route_t *path = paths->at(rnd);

    route_t *ackPath = getReversePath(src, dest, path);
    return make_pair(path, ackPath);
}

vector<route_t *>* AspenTree::get_paths_ecmp(int src, int dest) {
    vector<route_t *> *paths = new vector<route_t *>();
    route_t *routeout = NULL;
    if (HOST_POD_SWITCH(src) == HOST_POD_SWITCH(dest)) {
        routeout = new route_t();
        routeout->push_back(HostTXQueues[src]);

        routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

        routeout->push_back(HostRecvQueues[dest]);

        paths->push_back(routeout);
        return paths;

    } else if (HOST_POD(src) == HOST_POD(dest)) {
        //don't go up the hierarchy, stay in the pod only.
        int pod = HOST_POD(src);
        //there are K/2 paths between the source and the destination
        for (int upper = MIN_POD_ID(pod); upper <= MAX_POD_ID(pod); upper++) {
            //upper is nup

            routeout = new route_t();
            routeout->push_back(HostTXQueues[src]);

            routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

            routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

            routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

            routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);

            routeout->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(dest)]);

            routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)]);

            routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

            routeout->push_back(HostRecvQueues[dest]);

            paths->push_back(routeout);

        }
        return paths;
    } else if(src/(NHOST/2) == dest/(NHOST/2)){

        int pod = HOST_POD(src);
        for (int upper = MIN_POD_ID(pod); upper <= MAX_POD_ID(pod); upper++) {
            int redPodIndex = upper/(NK/2);
            int upPodOffset = upper %(K/2);

            for (int i = 0; i < K / 2; i++) {
                int red = redPodIndex*(NK/2)+upPodOffset*(K/2) + i;

                //upper is nup

                routeout = new route_t();
                routeout->push_back(HostTXQueues[src]);

                routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

                routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

                routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                routeout->push_back(pipes_nup_nred[upper][red]);

                routeout->push_back(queues_nup_nred[upper][red]);

                //now take the only link down to the destination server!

                int upper2 = HOST_POD(dest) * K / 2 + red%(NK/2)/(K/2);

                routeout->push_back(pipes_nred_nup[red][upper2]);

                routeout->push_back(queues_nred_nup[red][upper2]);

                routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

                routeout->push_back(HostRecvQueues[dest]);

                paths->push_back(routeout);
            }
        }
        return paths;
    }else{

        int pod = HOST_POD(src);
        for (int upper = MIN_POD_ID(pod); upper <= MAX_POD_ID(pod); upper++) {

            for(int ri = 0; ri < K/2; ri++){
                int redPodIndex = upper/(NK/2);
                int upPodOffset = upper %(K/2);
                int red = redPodIndex*(NK/2)+upPodOffset*(K/2) + ri;

                for (int ci = 0; ci< K/2; ci++){
                    int core = red % (K / 2) + ci*K/2;

                    routeout = new route_t();
                    routeout->push_back(HostTXQueues[src]);

                    routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

                    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

                    routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                    routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                    routeout->push_back(pipes_nup_nred[upper][red]);

                    routeout->push_back(queues_nup_nred[upper][red]);

                    routeout->push_back(pipes_nred_nc[red][core]);

                    routeout->push_back(queues_nred_nc[red][core]);

                    //a core switch have redundant links to every red pod, just randomly pick one

                    int red2 = dest/(NHOST/2)*(NK/2)+core%(K/2)+rand()%(K/2)*K/2;

                    int upper2 = HOST_POD(dest) * K / 2 +  red2%(NK/2) / (K/2);

                    routeout->push_back(pipes_nc_nred[core][red2]);

                    routeout->push_back(queues_nc_nred[core][red2]);

                    routeout->push_back(pipes_nred_nup[red2][upper2]);

                    routeout->push_back(queues_nred_nup[red2][upper2]);

                    routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                    routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                    routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

                    routeout->push_back(HostRecvQueues[dest]);

                    paths->push_back(routeout);

                }
            }
        }
        return paths;
    }
}

pair<route_t *, route_t *> AspenTree::getStandardPath(int src, int dest) {
    //right now we only support ecmp routing for aspen tree
    return getEcmpPath(src, dest);
}

pair<route_t *, route_t *> AspenTree::getReroutingPath(int src, int dest, route_t *currentPath) {
    return getOneWorkingPath(src,dest);
}



