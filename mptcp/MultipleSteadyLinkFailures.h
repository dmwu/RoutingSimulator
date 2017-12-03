//
// Created by Dingming Wu on 11/28/17.
//

#ifndef SIM_MULTIPLESTEADYLINKFAILURES_H
#define SIM_MULTIPLESTEADYLINKFAILURES_H


#include "eventlist.h"
#include "topology.h"
#include <algorithm>    // std::random_shuffle
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
#include "../main.h"
class MultipleSteadyLinkFailures{
public:
    MultipleSteadyLinkFailures(EventList*ev,Topology*topo);
    EventList*_ev;
    set<int>* _givenFailedLinks;
    set<int>* _givenFailedSwitches;
    vector<int>* _allLinks;
    vector<int>* _allSwitches;

    vector<int>* _inNetworkLinks;
    vector<int>* _inNetworkSwitches;

    int _totalLinks;
    int _totalSwitches;
    void setSingleLinkFailure(int linkid);
    void setRandomLinkFailures(int num);
    void setSingleSwitchFailure(int switchId);
    void setRandomSwitchFailure(int num);
    void setRandomSwitchFailure(double ratio);
    void setRandomLinkFailures(double ratio);
    Topology*_topo;
    bool isPathOverlapping(route_t*);
    set<int>* _outstandingFailedLinks;
    set<int>* _outstandingFailedSwitches;
    void installFailures();
    bool _useShareBackup =false;
    void updateBackupUsage();
};


#endif //SIM_MULTIPLESTEADYLINKFAILURES_H
