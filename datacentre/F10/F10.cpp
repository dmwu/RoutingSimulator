//
// Created by Dingming Wu on 11/8/17.
//

#include <iostream>
#include <random>
#include <algorithm>
#include "F10.h"

F10Topology::F10Topology(EventList *ev) {
    _eventlist = ev;
    _numLinks = K * K * K / 2 + NHOST; //[WDM] should include host links

    _failedLinks = new bool[_numLinks];
    for (int i = 0; i < _numLinks; i++) {
        _failedLinks[i] = false;
    }

    _net_paths = new vector<route_t *> **[NHOST];

    for (int i = 0; i < NHOST; i++) {
        _net_paths[i] = new vector<route_t *> *[NHOST];
        for (int j = 0; j < NHOST; j++)
            _net_paths[i][j] = NULL;
    }

    init_network();
}


void F10Topology::init_network() {


    for (int i = 0; i < NSRV; i++) {
        HostTXQueues[i] = new RandomQueue(speedFromPktps(HOST_NIC),
                                          memFromPkt(FEEDER_BUFFER + RANDOM_BUFFER),
                                          *_eventlist, NULL, memFromPkt(RANDOM_BUFFER),
                                          "TxQueue:" + ntoa(i), -1);
        HostTXQueues[i]->_isHostQueue=true;

        HostRecvQueues[i] = new RandomQueue(speedFromPktps(HOST_NIC),
                                            memFromPkt(FEEDER_BUFFER + RANDOM_BUFFER),
                                            *_eventlist, NULL, memFromPkt(RANDOM_BUFFER),
                                            "RxQueue:" + ntoa(i), -1);
        HostRecvQueues[i]->_isHostQueue=true;
    }

    for (int j = 0; j < NC; j++)
        for (int k = 0; k < NK; k++) {
            queues_nc_nup[j][k] = NULL;
            pipes_nc_nup[j][k] = NULL;
            queues_nup_nc[k][j] = NULL;
            pipes_nup_nc[k][j] = NULL;
        }

    for (int j = 0; j < NK; j++)
        for (int k = 0; k < NK; k++) {
            queues_nup_nlp[j][k] = NULL;
            pipes_nup_nlp[j][k] = NULL;
            queues_nlp_nup[k][j] = NULL;
            pipes_nlp_nup[k][j] = NULL;
        }

    for (int j = 0; j < NK; j++)
        for (int k = 0; k < NSRV; k++) {
            pipes_nlp_ns[j][k] = NULL;
            queues_ns_nlp[k][j] = NULL;
            pipes_ns_nlp[k][j] = NULL;
        }

    // lower layer pod switch to server
    for (int j = 0; j < NK; j++) {

        for (int l = 0; l < K * RATIO / 2; l++) {
            //[WDM] k is both the global hostID and linkID
            int k = j * K * RATIO / 2 + l;

            //[WDM]handle host queues separately
            pipes_nlp_ns[j][k] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-lp-host-" + ntoa(j) + "-" + ntoa(k));

            // Uplink
            queues_ns_nlp[k][j] = new RandomQueue(speedFromPktps(HOST_NIC),
                                                  memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                  *_eventlist, NULL, memFromPkt(RANDOM_BUFFER),
                                                  "Queue-host-lp-" + ntoa(k) + "-" + ntoa(j), j);

            pipes_ns_nlp[k][j] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-host-lp-" + ntoa(k) + "-" + ntoa(j));

            pipes_nlp_ns[j][k]->setDualPipe(pipes_ns_nlp[k][j]);
            queues_ns_nlp[k][j]->setDualQueue(HostRecvQueues[k]);

        }
    }
    //Lower layer in pod to upper layer in pod!
    for (int j = 0; j < NK; j++) {
        int podid = 2 * j / K;
        //Connect the lower layer switch to the upper layer switches in the same pod
        for (int k = MIN_POD_ID(podid); k <= MAX_POD_ID(podid); k++) {
            // Downlink

            queues_nup_nlp[k][j] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                   memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                   *_eventlist, NULL,
                                                   memFromPkt(RANDOM_BUFFER),
                                                   "Queue-up-lp-" + ntoa(k) + "-" + ntoa(j), j);

            pipes_nup_nlp[k][j] = new Pipe(timeFromUs(RTT), *_eventlist,
                                           "Pipe-up-lp-" + ntoa(k) + "-" + ntoa(j));

            // Uplink
            queues_nlp_nup[j][k] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                   memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                   *_eventlist, NULL,
                                                   memFromPkt(RANDOM_BUFFER),
                                                   "Queue-lp-up-" + ntoa(j) + "-" + ntoa(k), k + NK);

            pipes_nlp_nup[j][k] = new Pipe(timeFromUs(RTT), *_eventlist,
                                           "Pipe-lp-up-" + ntoa(j) + "-" + ntoa(k));

            pipes_nup_nlp[k][j]->setDualPipe(pipes_nlp_nup[j][k]);
            queues_nup_nlp[k][j]->setDualQueue(queues_nlp_nup[j][k]);
        }
    }


    // Upper layer in pod to core!
    for (int j = 0; j < NK; j++) {
        int podpos = j % (K / 2);
        int pod_id = j / (K / 2);
        for (int l = 0; l < K / 2; l++) {
            int k;
            if (pod_id % 2 == 0)
                k = podpos * K / 2 + l;
            else
                k = l * K / 2 + podpos;

            queues_nup_nc[j][k] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                  memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                  *_eventlist,
                                                  NULL, memFromPkt(RANDOM_BUFFER),
                                                  "Queue-up-co-" + ntoa(j) + "-" + ntoa(k), k + 2 * NK);


            pipes_nup_nc[j][k] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-up-co-" + ntoa(j) + "-" + ntoa(k));

            queues_nc_nup[k][j] = new RandomQueue(speedFromPktps(HOST_NIC / CORE_TO_HOST),
                                                  memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER),
                                                  *_eventlist, NULL,
                                                  memFromPkt(RANDOM_BUFFER),
                                                  "Queue-co-up-" + ntoa(k) + "-" + ntoa(j), j + NK);

            pipes_nc_nup[k][j] = new Pipe(timeFromUs(RTT), *_eventlist,
                                          "Pipe-co-up-" + ntoa(k) + "-" + ntoa(j));

            queues_nup_nc[j][k]->setDualQueue(queues_nc_nup[k][j]);
            pipes_nc_nup[k][j]->setDualPipe(pipes_nup_nc[j][k]);

        }
    }
}

vector<route_t *> *F10Topology::get_paths_ecmp(int src, int dest) {
    vector<route_t *> *paths = new vector<route_t *>();
    route_t *routeout;
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
    } else {
        int pod1 = HOST_POD(src);
        int pod2 = HOST_POD(dest);
        for (int upper = MIN_POD_ID(pod1); upper <= MAX_POD_ID(pod1); upper++) {
            int core = -1;
            for (int i = 0; i < K / 2; i++) {
                if (pod1 % 2 == 0)
                    core = (upper % (K / 2)) * (K / 2) + i % (K / 2);
                else
                    core = upper % (K / 2) + i * (K / 2);

                routeout = new route_t();
                routeout->push_back(HostTXQueues[src]);

                routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

                routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

                routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper]);

                routeout->push_back(pipes_nup_nc[upper][core]);

                routeout->push_back(queues_nup_nc[upper][core]);

                //now take the only link down to the destination server!

                int upper2 = -1;
                if (pod2 % 2 == 0)
                    upper2 = pod2 * K / 2 + core / (K / 2);
                else
                    upper2 = pod2 * K / 2 + core % (K / 2);

                routeout->push_back(pipes_nc_nup[core][upper2]);

                routeout->push_back(queues_nc_nup[core][upper2]);

                routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)]);

                routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

                routeout->push_back(HostRecvQueues[dest]);

                paths->push_back(routeout);
            }
        }
        return paths;
    }
}

route_t *F10Topology::get_path_2levelrt(int src, int dest) {

    route_t *routeout = new route_t();
    RandomQueue *txqueue = HostTXQueues[src];
    routeout->push_back(txqueue);

    //under the same TOR
    if (HOST_POD_SWITCH(src) == HOST_POD_SWITCH(dest)) {

        routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);
        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

        //under the same Pod
    } else if (HOST_POD(src) == HOST_POD(dest)) {
        int pod = HOST_POD(src);
        int srcHostID = src % (K / 2);
        int destHostID = dest % (K / 2);
        int aggSwitchID = MIN_POD_ID(pod) + srcHostID;

        routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][aggSwitchID]);


        routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][aggSwitchID]);

        routeout->push_back(pipes_nup_nlp[aggSwitchID][HOST_POD_SWITCH(dest)]);

        routeout->push_back(queues_nup_nlp[aggSwitchID][HOST_POD_SWITCH(dest)]);

        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);


    } else {

        int pod1 = HOST_POD(src);
        int pod2 = HOST_POD(dest);
        int srcHostID = src % (K / 2); //could be used as outgoing offset
        int destHostID = dest % (K / 2);
        int aggSwitchID1 = MIN_POD_ID(pod1) + srcHostID;
        int core = -1;
        int aggSwitchID2 = -1;
        //[wdm] For now don't use core offset to diffuse traffic of incast
        if (pod1 % 2 == 0)
            core = (aggSwitchID1 % (K / 2)) * (K / 2) + (destHostID) % (K / 2);
        else
            core = aggSwitchID1 % (K / 2) + destHostID * (K / 2);

        if (pod2 % 2 == 0)
            aggSwitchID2 = pod2 * K / 2 + core / (K / 2);
        else
            aggSwitchID2 = pod2 * K / 2 + core % (K / 2);

        routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)]);

        routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][aggSwitchID1]);

        routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][aggSwitchID1]);

        routeout->push_back(pipes_nup_nc[aggSwitchID1][core]);

        routeout->push_back(queues_nup_nc[aggSwitchID1][core]);

        routeout->push_back(pipes_nc_nup[core][aggSwitchID2]);

        routeout->push_back(queues_nc_nup[core][aggSwitchID2]);

        routeout->push_back(pipes_nup_nlp[aggSwitchID2][HOST_POD_SWITCH(dest)]);

        routeout->push_back(queues_nup_nlp[aggSwitchID2][HOST_POD_SWITCH(dest)]);

        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);

    }
    RandomQueue *rxQueue = HostRecvQueues[dest];
    routeout->push_back(rxQueue);
    return routeout;
}


route_t *F10Topology::getAlternativePath(int src, int dest, route_t *dataPath) {
    route_t* routeout= nullptr;
    int failurePos = -1;
    for (int i = 0; i < dataPath->size(); i++) {
        PacketSink *t = dataPath->at(i);
        if (t->_disabled) {
            failurePos = i;
            break;
        }
    }
    if (failurePos <= 2 || failurePos >= dataPath->size() - 2) {
        //host link failures, no alternative
        return nullptr;
    }

    PacketSink *ff = dataPath->at(failurePos);
    int failedSwitch = ff->_switchId;
    int dualSwitch = ff->getDual()->_switchId;

    if (dualSwitch > failedSwitch && failedSwitch < NK) {
        //last-hop agg->tor failure
        int destTorIndex = failedSwitch;
        int aggSwitchIndex = dualSwitch - NK;
        int pod = HOST_POD(dest);
        for (int tor = MIN_POD_ID(pod); tor <= MAX_POD_ID(pod) && tor != destTorIndex; tor++) {
            for (int upper = MIN_POD_ID(pod); upper <= MAX_POD_ID(pod) && upper != aggSwitchIndex; upper++) {
                if (!queues_nup_nlp[aggSwitchIndex][tor]->_disabled
                    && !queues_nlp_nup[tor][upper]->_disabled
                    && !queues_nup_nlp[upper][destTorIndex]->_disabled) {
                    routeout = new route_t(dataPath->begin(), dataPath->begin() + failurePos - 2 + 1);
                    routeout->push_back(pipes_nup_nlp[aggSwitchIndex][tor]);
                    routeout->push_back(queues_nup_nlp[aggSwitchIndex][tor]);
                    routeout->push_back(pipes_nlp_nup[tor][upper]);
                    routeout->push_back(queues_nlp_nup[tor][upper]);
                    routeout->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(dest)]);
                    routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)]);
                    routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
                    routeout->push_back(HostRecvQueues[dest]);
                    return routeout;
                }

            }
        }
    } else if (dualSwitch > failedSwitch && failedSwitch >= NK) {
        //core to agg link failure
        int coreIndex = dualSwitch - 2 * NK;
        int desAggIndex = failedSwitch- NK;
        int pod = HOST_POD(dest);
        if (pod % 2 == 0) {
            for (int coreLinkIndex = 1; coreLinkIndex < K; coreLinkIndex += 2) {
                int newAggIndex = coreLinkIndex * K / 2 + coreIndex % (K / 2);
                for (int aggLinkIndex = 0; aggLinkIndex < K / 2; aggLinkIndex++) {
                    int newCore = aggLinkIndex * K / 2 + newAggIndex % (K / 2);
                    int newNewAgg = pod*K/2 + newCore / (K / 2);
                    if (newCore != coreIndex
                        && !queues_nc_nup[coreIndex][newAggIndex]->_disabled
                        && !queues_nup_nc[newAggIndex][newCore]->_disabled
                        && !queues_nc_nup[newCore][newNewAgg]->_disabled
                        && !queues_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]->_disabled) {
                        assert(desAggIndex != newNewAgg && desAggIndex / (K / 2) == newNewAgg / (K / 2));
                        routeout = new route_t(dataPath->begin(), dataPath->begin() + failurePos - 2 + 1);
                        routeout->push_back(pipes_nc_nup[coreIndex][newAggIndex]);
                        routeout->push_back(queues_nc_nup[coreIndex][newAggIndex]);
                        routeout->push_back(pipes_nup_nc[newAggIndex][newCore]);
                        routeout->push_back(queues_nup_nc[newAggIndex][newCore]);
                        routeout->push_back(pipes_nc_nup[newCore][newNewAgg]);
                        routeout->push_back(queues_nc_nup[newCore][newNewAgg]);
                        routeout->push_back(pipes_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]);
                        routeout->push_back(queues_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]);
                        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
                        routeout->push_back(HostRecvQueues[dest]);
                        return routeout;
                    }
                }
            }
        } else { //pod%2==1
            for (int coreLinkIndex = 0; coreLinkIndex < K; coreLinkIndex += 2) {
                int newAggIndex = coreLinkIndex * K / 2 + coreIndex / (K / 2);
                for (int aggLinkIndex = 0; aggLinkIndex < K / 2; aggLinkIndex++) {
                    int newCore = aggLinkIndex + (newAggIndex % (K / 2)) * (K / 2);
                    int newNewAgg = pod*K/2 + newCore % (K / 2);
                    if (newCore != coreIndex
                        && !queues_nc_nup[coreIndex][newAggIndex]->_disabled
                        && !queues_nup_nc[newAggIndex][newCore]->_disabled
                        && !queues_nc_nup[newCore][newNewAgg]->_disabled
                        && !queues_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]->_disabled) {
                        assert(desAggIndex != newNewAgg && desAggIndex / (K / 2) == newNewAgg / (K / 2));
                        routeout = new route_t(dataPath->begin(), dataPath->begin() + failurePos - 2 + 1);
                        routeout->push_back(pipes_nc_nup[coreIndex][newAggIndex]);
                        routeout->push_back(queues_nc_nup[coreIndex][newAggIndex]);
                        routeout->push_back(pipes_nup_nc[newAggIndex][newCore]);
                        routeout->push_back(queues_nup_nc[newAggIndex][newCore]);
                        routeout->push_back(pipes_nc_nup[newCore][newNewAgg]);
                        routeout->push_back(queues_nc_nup[newCore][newNewAgg]);
                        routeout->push_back(pipes_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]);
                        routeout->push_back(queues_nup_nlp[newNewAgg][HOST_POD_SWITCH(dest)]);
                        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest]);
                        routeout->push_back(HostRecvQueues[dest]);
                        return routeout;
                    }
                }
            }
        }
    } else {
        //upward link failure
        if (_net_paths[src][dest] == nullptr)
            _net_paths[src][dest] = get_paths_ecmp(src, dest);
        for (route_t *path: *_net_paths[src][dest]) {
            if (isPathValid(path)) {
                return path;
            }
        }
    }

    return nullptr;
}

pair<route_t *, route_t *> F10Topology::getStandardPath(int src, int dest) {

    route_t *path = get_path_2levelrt(src, dest);
    if (isPathValid(path)) {
        route_t *ackPath = getReversePath(src,dest,path);
        if (isPathValid(ackPath)) {
            return make_pair(path, ackPath);
        }
    }
    return make_pair(nullptr, nullptr);

}

pair<route_t *, route_t *> F10Topology::getReroutingPath(int src, int dest, route_t* currentPath) {
    if (_net_paths[src][dest] == NULL) {
        _net_paths[src][dest] = get_paths_ecmp(src, dest);
    }
    route_t* path = getAlternativePath(src, dest, currentPath);
    if (isPathValid(path)) {
        route_t *ackPath = getReversePath(src, dest, path);
        if (isPathValid(ackPath)) {
            return make_pair(path, ackPath);
        }
    }

    return make_pair(nullptr, nullptr);

}

pair<route_t*, route_t*> F10Topology::getEcmpPath(int src, int dest) {
    if (_net_paths[src][dest] == NULL) {
        _net_paths[src][dest] = get_paths_ecmp(src, dest);
    }
    vector<route_t *> *paths = _net_paths[src][dest];
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(*paths), std::end(*paths), rng);
    for (route_t *path: *paths) {
        if (isPathValid(path)) {
            route_t* ackPath = getReversePath(src, dest, path);
            if (isPathValid(ackPath)) {
                return make_pair(path, ackPath);
            }
        }
    }
    return make_pair(nullptr, nullptr);
}

route_t* F10Topology::getReversePath(int src, int dest, route_t *dataPath) {
    route_t *ackPath = new route_t();
    assert(dataPath->size()>=2);
    for(int i = dataPath->size()-2; i>=0; i-=2){
        PacketSink* dualQueue = dataPath->at(i+1)->getDual();
        PacketSink* dualPipe = dataPath->at(i)->getDual();
        ackPath->push_back(dualPipe);
        ackPath->push_back(dualQueue);
    }
    ackPath->insert(ackPath->begin(), HostTXQueues[dest]);
    return ackPath;
}

pair<Queue *, Queue *> F10Topology::linkToQueues(int linkid) {
    pair<Queue *, Queue *> ret(nullptr, nullptr);
    int lower, higher;
    if (linkid < NHOST) {
        lower = linkid;
        higher = linkid / (K / 2);
        ret.first = queues_ns_nlp[lower][higher];
        ret.second = HostRecvQueues[lower];
        return ret;
    } else if (linkid >= NHOST && linkid < 2 * NHOST) {
        linkid -= NHOST;
        lower = linkid / (K / 2);
        higher = MIN_POD_ID(lower/(K/2)) + linkid % (K / 2);
        ret.first = queues_nlp_nup[lower][higher];
        ret.second = queues_nup_nlp[higher][lower];

    } else {
        assert(linkid >= 2 * NHOST && linkid < 3 * NHOST);
        linkid -= 2 * NHOST;
        lower = linkid / (K / 2);
        int pod = linkid / (K * K / 4);
        if (pod % 2 == 0)
            higher = (lower % (K / 2)) * (K / 2) + linkid % (K / 2);
        else
            higher = lower % (K / 2) + (linkid % (K / 2)) * (K / 2);

        ret.first = queues_nup_nc[lower][higher];
        ret.second = queues_nc_nup[higher][lower];
    }
    if(ret.first == nullptr || ret.second == nullptr){
        std::cout<<"invalid linkid for failure:"<<linkid<<endl;
        exit(-1);
    }
    return ret;
}


int F10Topology::getServerUnderTor(int torIndex, int torNum) {
    int total = K * K / 2;
    int mappedTor = (int) ((double) torIndex / torNum * total);

    int result = rand_host_sw(mappedTor);

    return result;
}

int F10Topology::rand_host_sw(int sw) {
    int hostPerEdge = K / 2;
    int server_sw = rand() % hostPerEdge;
    int server = sw * hostPerEdge + server_sw;
    return server;
}

void F10Topology::failLink(int linkid) {
    if (_failedLinks[linkid]) {
        return;
    }
    pair<Queue *, Queue *> queues = linkToQueues(linkid);
    Queue *upLinkQueue = queues.first;
    Queue *downLinkQueue = queues.second;
    assert(!upLinkQueue->_disabled && !downLinkQueue->_disabled);

    _failedLinks[linkid] = true;
    upLinkQueue->_disabled = true;
    downLinkQueue->_disabled = true;
}

void F10Topology::recoverLink(int linkid) {
    if (!_failedLinks[linkid]) {
        return;
    }
    pair<Queue *, Queue *> queues = linkToQueues(linkid);
    Queue *upLinkQueue = queues.first;
    Queue *downLinkQueue = queues.second;
    assert(upLinkQueue->_disabled && downLinkQueue->_disabled);

    _failedLinks[linkid] = false;
    upLinkQueue->_disabled = false;
    downLinkQueue->_disabled = false;
}

int F10Topology::nodePair_to_link(int node1, int node2) {

    int result;
    if (node1 > node2) {
        swap(node1, node2);
    }
    if (node1 < NK) {
        int aggr_in_pod = (node2 - NK) % (K / 2);
        result = (K * node1 / 2 + aggr_in_pod);
    } else {
        int core_in_group = (node2 - 2 * NK) % (K / 2);
        result = (K * NK / 2 + (node1 - NK) * K / 2 + core_in_group);
    }
    return result;
}

bool F10Topology::isPathValid(route_t *rt) {
    if (rt == NULL) return false;
    for (int i = 0; i < rt->size(); i += 2) {
        if (rt->at(i)->_disabled) {
            return false;
        }
    }
    return true;
}







